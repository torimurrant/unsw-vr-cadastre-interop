using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Xml;
using System.Text.RegularExpressions;
using System.Diagnostics;

public class KMLImporter : MonoBehaviour
{
    [Header("File")]
    [Tooltip("File Path")]
    public string kmlFilePath = @"ADD FILE PATH HERE";
    [Header("Wireframe colour")]
    public Color wireColour = new Color(1.0f, 0.6f, 0.2f, 1.0f);

    [Header("Geometry")]
    [Tooltip("Total vertices")]
    public int totalVertices = 0;
    [Tooltip("Total triangulated faces")]
    public int totalFaces = 0;
    [Tooltip("Total polygon rings")]
    public int totalPolygons = 0;
    [Tooltip("Degenerate faces detected")]
    public int degenerateFaces = 0;
    [Tooltip("Geometric errors detected")]
    public string geometricErrors = "None detected";

    [Header("Semantic")]
    [Tooltip("Attribute fields present")]
    public string attributeFieldsRetained = "";

    [Header("Performance")]
    [Tooltip("Import time")]
    public float importTimeSeconds = 0f;
    [Tooltip("Frames per second")]
    public float currentFPS = 0f;
    [Tooltip("RAM usage (mb)")]
    public float ramUsageMB = 0f;
    
    private double originLon = 151.76675;
    private double originLat = -32.92564;
    private double originAlt = 0.0;
    private const double MetresPerDegLon = 88900.0;
    private const double MetresPerDegLat = 111320.0;
    private List<Vector3[]> allEdges = new List<Vector3[]>();
    private Material lineMat;
    private bool loadComplete = false;
    private float fpsTimer = 0f;
    private int fpsFrameCount = 0;

    void Start()
    {
        baselineRAM_MB = GetRAM_MB();

        lineMat = new Material(Shader.Find("Hidden/Internal-Colored"));
        lineMat.hideFlags = HideFlags.HideAndDontSave;
        lineMat.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
        lineMat.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
        lineMat.SetInt("_Cull", (int)UnityEngine.Rendering.CullMode.Off);
        lineMat.SetInt("_ZWrite", 0);

        StartCoroutine(LoadKML());
    }

    void Update()
    {
        if (!loadComplete) return;

        fpsTimer += Time.unscaledDeltaTime;
        fpsFrameCount++;
        if (fpsTimer >= 0.5f)
        {
            currentFPS = fpsFrameCount / fpsTimer;
            fpsTimer = 0f;
            fpsFrameCount = 0;
        }

        ramUsageMB = GetRAM_MB();
    }

    IEnumerator LoadKML()
    {
        Stopwatch sw = Stopwatch.StartNew();
        string kmlContent = "";
        string ext = Path.GetExtension(kmlFilePath).ToLower();

        if (ext == ".kmz")
        {
            using (ZipArchive zip = ZipFile.OpenRead(kmlFilePath))
            {
                foreach (ZipArchiveEntry entry in zip.Entries)
                {
                    if (entry.Name.EndsWith(".kml"))
                    {
                        using (StreamReader sr = new StreamReader(entry.Open()))
                            kmlContent = sr.ReadToEnd();
                        break;
                    }
                }
            }
        }
        else
        {
            kmlContent = File.ReadAllText(kmlFilePath);
        }

        XmlDocument doc = new XmlDocument();
        doc.LoadXml(kmlContent);

        XmlNamespaceManager ns = new XmlNamespaceManager(doc.NameTable);
        ns.AddNamespace("kml", "http://www.opengis.net/kml/2.2");

        XmlNode firstAltMode = doc.SelectSingleNode("//*[local-name()='altitudeMode']");
        altitudeMode = firstAltMode != null ? firstAltMode.InnerText.Trim() : "Not specified (defaults to clampToGround)";

        double minLon = double.MaxValue, maxLon = double.MinValue;
        double minLat = double.MaxValue, maxLat = double.MinValue;
        double minAlt = double.MaxValue, maxAlt = double.MinValue;
        bool boundsFound = false;

        XmlNode docNode = doc.SelectSingleNode("//*[local-name()='Document']");
        XmlNode folderNode = doc.SelectSingleNode("//*[local-name()='Folder']");

        string docName = docNode?.SelectSingleNode("*[local-name()='name']")?.InnerText.Trim() ?? "Document";
        string folderName = folderNode?.SelectSingleNode("*[local-name()='name']")?.InnerText.Trim() ?? "Folder";
        int folderCount = doc.SelectNodes("//*[local-name()='Folder']")?.Count ?? 0;

        XmlNodeList placemarks = doc.SelectNodes("//*[local-name()='Placemark']");

        HashSet<string> sourceFields = new HashSet<string>();
        HashSet<string> retainedFields = new HashSet<string>();

        XmlNode firstPM = placemarks[0];
        XmlNode firstDesc = firstPM.SelectSingleNode("*[local-name()='description']");
        if (firstDesc != null)
        {
            Dictionary<string, string> firstAttrs = ParseDescriptionHTML(firstDesc.InnerText);
            foreach (string k in firstAttrs.Keys) sourceFields.Add(k);
        }

        sourceFields.Add("name");

        attributeFieldsInSource = sourceFields.Count > 0
            ? string.Join(", ", sourceFields)
            : "None (attributes embedded in non-machine-readable HTML CDATA — not accessible as structured data)";
            
        GameObject folderGO = new GameObject(folderName);
        folderGO.transform.SetParent(transform);

        int pmIndex = 0;

        foreach (XmlNode pm in placemarks)
        {
            XmlNode nameNode = pm.SelectSingleNode("*[local-name()='name']");
            string pmName = nameNode?.InnerText.Trim() ?? $"Placemark_{pmIndex}";

            XmlNode descNode = pm.SelectSingleNode("*[local-name()='description']");
            Dictionary<string, string> attrs = new Dictionary<string, string>();
            if (descNode != null)
                attrs = ParseDescriptionHTML(descNode.InnerText);

            foreach (string k in attrs.Keys) retainedFields.Add(k);
            retainedFields.Add("name");

            XmlNode styleUrlNode = pm.SelectSingleNode("*[local-name()='styleUrl']");
            string styleUrl = styleUrlNode?.InnerText.Trim() ?? "";

            GameObject pmGO = new GameObject(pmName);
            pmGO.transform.SetParent(folderGO.transform);

            KMLMetadata meta = pmGO.AddComponent<KMLMetadata>();
            meta.placemarkName = pmName;
            meta.styleUrl = styleUrl;
            meta.attributes = attrs;

            XmlNodeList polygonNodes = pm.SelectNodes(".//*[local-name()='Polygon']");
            int polyIndex = 0;

            foreach (XmlNode polygon in polygonNodes)
            {
                XmlNode coordNode = polygon.SelectSingleNode(
                    ".//*[local-name()='outerBoundaryIs']//*[local-name()='coordinates']");
                if (coordNode == null) continue;

                List<Vector3> verts = ParseCoordinates(coordNode.InnerText.Trim(),
                    ref minLon, ref maxLon, ref minLat, ref maxLat, ref minAlt, ref maxAlt);
                boundsFound = true;

                if (verts == null || verts.Count < 3) continue;

                if (verts.Count > 3 &&
                    Vector3.Distance(verts[0], verts[verts.Count - 1]) < 0.001f)
                    verts.RemoveAt(verts.Count - 1);

                if (verts.Count < 3) continue;
                
                for (int i = 0; i < verts.Count; i++)
                {
                    Vector3 a = pmGO.transform.TransformPoint(verts[i]);
                    Vector3 b = pmGO.transform.TransformPoint(verts[(i + 1) % verts.Count]);
                    allEdges.Add(new Vector3[] { a, b });
                }

                List<int> triangles = new List<int>();
                for (int i = 1; i < verts.Count - 1; i++)
                {
                    triangles.Add(0); triangles.Add(i); triangles.Add(i + 1);
                    if (verts[0] == verts[i] || verts[i] == verts[i + 1] || verts[0] == verts[i + 1])
                        degenerateFaces++;
                }

                Mesh mesh = new Mesh();
                mesh.name = $"{pmName}_poly{polyIndex}";
                mesh.vertices = verts.ToArray();
                mesh.triangles = triangles.ToArray();
                mesh.RecalculateNormals();
                mesh.RecalculateBounds();

                GameObject polyGO = new GameObject($"Polygon_{polyIndex}");
                polyGO.transform.SetParent(pmGO.transform);

                MeshFilter mf = polyGO.AddComponent<MeshFilter>();
                MeshRenderer mr = polyGO.AddComponent<MeshRenderer>();
                mf.sharedMesh = mesh;

                Material mat = new Material(Shader.Find("Hidden/Internal-Colored"));
                mat.color = Color.clear;
                mr.sharedMaterial = mat;

                totalVertices += verts.Count;
                totalFaces += triangles.Count / 3;
                totalPolygons++;
                polyIndex++;
            }

            placemarksLoaded++;
            pmIndex++;

            if (pmIndex % 10 == 0)
            {
                UnityEngine.Debug.Log($"KMLLoader: Processed {pmIndex} placemarks...");
                yield return null;
            }
        }

        sw.Stop();
        importTimeSeconds = (float)(sw.Elapsed.TotalSeconds);

        estimatedEdges = (int)(totalFaces * 1.5f);

        if (boundsFound)
        {
            double cx = (minLon + maxLon) / 2.0;
            double cy = (minLat + maxLat) / 2.0;
            double cz = (minAlt + maxAlt) / 2.0;
            sourceCentroid = $"Lon: {cx:F6}, Lat: {cy:F6}, Alt: {cz:F2}m";
        }

        unityCentroid = CalculateCentroid();

        attributeFieldsRetained = retainedFields.Count > 0
            ? string.Join(", ", retainedFields)
            : "None";

        int sourceCount = sourceFields.Count;
        int retainedCount = retainedFields.Count;

        loadComplete = true;

        UnityEngine.Debug.Log("!Load Complete!");
        UnityEngine.Debug.Log($"Import time:      {importTimeSeconds:F2}s");
        UnityEngine.Debug.Log($"Total vertices:   {totalVertices}");
        UnityEngine.Debug.Log($"Total faces:      {totalFaces}");
        UnityEngine.Debug.Log("!=======================!");
    }

    void OnRenderObject()
    {
        if (allEdges == null || allEdges.Count == 0 || lineMat == null) return;

        lineMat.SetPass(0);
        GL.Begin(GL.LINES);
        GL.Color(wireColour);

        foreach (Vector3[] edge in allEdges)
        {
            GL.Vertex(edge[0]);
            GL.Vertex(edge[1]);
        }

        GL.End();
    }

    // ── Parse KML coordinates (lon,lat,alt space-separated) ───────────────────
    List<Vector3> ParseCoordinates(string coordText,
        ref double minLon, ref double maxLon,
        ref double minLat, ref double maxLat,
        ref double minAlt, ref double maxAlt)
    {
        string[] tokens = coordText.Split(
            new char[] { ' ', '\t', '\n', '\r' },
            System.StringSplitOptions.RemoveEmptyEntries);

        List<Vector3> verts = new List<Vector3>();

        foreach (string token in tokens)
        {
            string[] parts = token.Split(',');
            if (parts.Length < 2) continue;

            if (!double.TryParse(parts[0], System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture, out double lon)) continue;
            if (!double.TryParse(parts[1], System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture, out double lat)) continue;
            double alt = 0.0;
            if (parts.Length > 2)
                double.TryParse(parts[2], System.Globalization.NumberStyles.Float,
                    System.Globalization.CultureInfo.InvariantCulture, out alt);

            // Update bounds
            if (lon < minLon) minLon = lon;
            if (lon > maxLon) maxLon = lon;
            if (lat < minLat) minLat = lat;
            if (lat > maxLat) maxLat = lat;
            if (alt < minAlt) minAlt = alt;
            if (alt > maxAlt) maxAlt = alt;

            float x = (float)((lon - originLon) * MetresPerDegLon);
            float y = (float)(alt - originAlt);
            float z = (float)((lat - originLat) * MetresPerDegLat);

            verts.Add(new Vector3(x, y, z));
        }

        return verts;
    }

    Dictionary<string, string> ParseDescriptionHTML(string html)
    {
        Dictionary<string, string> result = new Dictionary<string, string>();
        MatchCollection matches = Regex.Matches(html,
            @"<td[^>]*>\s*([A-Za-z_][A-Za-z_0-9]*)\s*</td>\s*<td[^>]*>\s*([^<]*?)\s*</td>",
            RegexOptions.IgnoreCase);

        foreach (Match m in matches)
        {
            string key = m.Groups[1].Value.Trim();
            string val = m.Groups[2].Value.Trim();
            if (!string.IsNullOrEmpty(key) && !result.ContainsKey(key))
                result[key] = val;
        }
        return result;
    }

    Vector3 CalculateCentroid()
    {
        MeshFilter[] meshFilters = GetComponentsInChildren<MeshFilter>(true);
        if (meshFilters.Length == 0) return Vector3.zero;
        Bounds b = new Bounds();
        bool init = false;
        foreach (MeshFilter mf in meshFilters)
        {
            if (mf.sharedMesh == null) continue;
            Bounds wb = new Bounds(
                mf.transform.TransformPoint(mf.sharedMesh.bounds.center),
                mf.sharedMesh.bounds.size);
            if (!init) { b = wb; init = true; }
            else b.Encapsulate(wb);
        }
        return b.center;
    }

    float GetRAM_MB()
    {
        return UnityEngine.Profiling.Profiler.GetTotalReservedMemoryLong() / (1024f * 1024f);
    }
}

public class KMLMetadata : MonoBehaviour
{
    public string placemarkName;
    public string styleUrl;
    public List<string> attributeKeys = new List<string>();
    public List<string> attributeValues = new List<string>();

    private Dictionary<string, string> _attributes;
    public Dictionary<string, string> attributes
    {
        get { return _attributes; }
        set
        {
            _attributes = value;
            attributeKeys.Clear();
            attributeValues.Clear();
            if (value != null)
                foreach (var kvp in value)
                {
                    attributeKeys.Add(kvp.Key);
                    attributeValues.Add(kvp.Value);
                }
        }
    }
}
