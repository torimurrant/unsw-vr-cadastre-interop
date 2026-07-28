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
    [Tooltip("File path")]
    public string kmlFilePath = @"ADD FILE PATH HERE";
    [Header("Wireframe colour")]
    public Color wireColour = new Color(1.0f, 0.6f, 0.2f, 1.0f);

    [Header("Geometry")]
    [Tooltip("Total vertices across all polygon rings")]
    public int totalVertices = 0;
    [Tooltip("Total triangulated faces")]
    public int totalFaces = 0;

    [Header("Semantics")]
    [Tooltip("Attribute fields present in source")]
    public string attributeFieldsInSource = "";
    [Tooltip("Attribute fields retained")]
    public string attributeFieldsRetained = "";

    [Header("Performance")]
    [Tooltip("Import time (seconds)")]
    public float importTimeSeconds = 0f;

    private double originLon = 151.76675;
    private double originLat = -32.92564;
    private double originAlt = 0.0;
    private const double MetresPerDegLon = 88900.0;
    private const double MetresPerDegLat = 111320.0;

    private List<Vector3[]> allEdges = new List<Vector3[]>();
    private Material lineMat;
    private bool loadComplete = false;

    void Start()
    {
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

    }

    IEnumerator LoadKML()
    {
        Stopwatch sw = Stopwatch.StartNew();

        string kmlContent = "";
        string ext = Path.GetExtension(kmlFilePath).ToLower();
        if (ext == ".kmz")
        {
            using (ZipArchive zip = ZipFile.OpenRead(kmlFilePath))
                foreach (ZipArchiveEntry entry in zip.Entries)
                    if (entry.Name.EndsWith(".kml"))
                    {
                        using (StreamReader sr = new StreamReader(entry.Open()))
                            kmlContent = sr.ReadToEnd();
                        break;
                    }
        }
        else
        {
            kmlContent = File.ReadAllText(kmlFilePath);
        }

        XmlDocument doc = new XmlDocument();
        doc.LoadXml(kmlContent);

        XmlNode folderNode = doc.SelectSingleNode("//*[local-name()='Folder']");
        string folderName = folderNode?.SelectSingleNode("*[local-name()='name']")?.InnerText.Trim() ?? "Folder";

        XmlNodeList placemarks = doc.SelectNodes("//*[local-name()='Placemark']");

        HashSet<string> sourceFields = new HashSet<string>();
        HashSet<string> retainedFields = new HashSet<string>();

        if (placemarks.Count > 0)
        {
            XmlNode firstDesc = placemarks[0].SelectSingleNode("*[local-name()='description']");
            if (firstDesc != null)
                foreach (string k in ParseDescriptionHTML(firstDesc.InnerText).Keys)
                    sourceFields.Add(k);
            sourceFields.Add("name");
        }

        attributeFieldsInSource = sourceFields.Count > 0
            ? string.Join(", ", sourceFields)
            : "None (attributes embedded in non-machine-readable HTML — not accessible as structured data)";

        GameObject folderGO = new GameObject(folderName);
        folderGO.transform.SetParent(transform);

        int pmIndex = 0;
        foreach (XmlNode pm in placemarks)
        {
            string pmName = pm.SelectSingleNode("*[local-name()='name']")?.InnerText.Trim() ?? $"Placemark_{pmIndex}";

            XmlNode descNode = pm.SelectSingleNode("*[local-name()='description']");
            Dictionary<string, string> attrs = new Dictionary<string, string>();
            if (descNode != null)
                attrs = ParseDescriptionHTML(descNode.InnerText);
            foreach (string k in attrs.Keys) retainedFields.Add(k);
            retainedFields.Add("name");

            GameObject pmGO = new GameObject(pmName);
            pmGO.transform.SetParent(folderGO.transform);

            KMLMetadata meta = pmGO.AddComponent<KMLMetadata>();
            meta.placemarkName = pmName;
            meta.attributes = attrs;

            XmlNodeList polygonNodes = pm.SelectNodes(".//*[local-name()='Polygon']");
            int polyIndex = 0;
            foreach (XmlNode polygon in polygonNodes)
            {
                XmlNode coordNode = polygon.SelectSingleNode(
                    ".//*[local-name()='outerBoundaryIs']//*[local-name()='coordinates']");
                if (coordNode == null) continue;

                List<Vector3> verts = ParseCoordinates(coordNode.InnerText.Trim());
                if (verts == null || verts.Count < 3) continue;

                if (verts.Count > 3 && Vector3.Distance(verts[0], verts[verts.Count - 1]) < 0.001f)
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
                }

                Mesh mesh = new Mesh();
                mesh.name = $"{pmName}_poly{polyIndex}";
                mesh.vertices = verts.ToArray();
                mesh.triangles = triangles.ToArray();
                mesh.RecalculateNormals();
                mesh.RecalculateBounds();

                GameObject polyGO = new GameObject($"Polygon_{polyIndex}");
                polyGO.transform.SetParent(pmGO.transform);
                polyGO.AddComponent<MeshFilter>().sharedMesh = mesh;
                var mr = polyGO.AddComponent<MeshRenderer>();
                var mat = new Material(Shader.Find("Hidden/Internal-Colored"));
                mat.color = Color.clear;
                mr.sharedMaterial = mat;

                totalVertices += verts.Count;
                totalFaces += triangles.Count / 3;
                polyIndex++;
            }

            pmIndex++;
            if (pmIndex % 10 == 0)
            {
                UnityEngine.Debug.Log($"KMLImporter: Processed {pmIndex} placemarks...");
                yield return null;
            }
        }

        sw.Stop();
        importTimeSeconds = (float)sw.Elapsed.TotalSeconds;
        attributeFieldsRetained = retainedFields.Count > 0 ? string.Join(", ", retainedFields) : "None";

        loadComplete = true;
        UnityEngine.Debug.Log("!Load complete!");
        UnityEngine.Debug.Log($"Import time: {importTimeSeconds:F2}s");
        UnityEngine.Debug.Log($"Total vertices: {totalVertices}");
        UnityEngine.Debug.Log($"Total faces: {totalFaces}");
        UnityEngine.Debug.Log("!=======================!");
    }

    void OnRenderObject()
    {
        if (allEdges == null || allEdges.Count == 0 || lineMat == null) return;
        lineMat.SetPass(0);
        GL.Begin(GL.LINES);
        GL.Color(wireColour);
        foreach (Vector3[] edge in allEdges) { GL.Vertex(edge[0]); GL.Vertex(edge[1]); }
        GL.End();
    }

    List<Vector3> ParseCoordinates(string coordText)
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

}

public class KMLMetadata : MonoBehaviour
{
    public string placemarkName;
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
                foreach (var kvp in value) { attributeKeys.Add(kvp.Key); attributeValues.Add(kvp.Value); }
        }
    }
}
