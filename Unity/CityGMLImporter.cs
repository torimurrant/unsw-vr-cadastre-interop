using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Diagnostics;

public class CityGMLImporter : MonoBehaviour
{
    [Header("File")]
    [Tooltip("File path")]
    public string gmlFilePath = @"ADD FILE PATH HERE";
    [Header("Wireframe colour")]
    public Color wireColour = new Color(0.3f, 0.8f, 1.0f, 1.0f);

    [Header("Geometry")]
    [Tooltip("Total vertices across all polygons")]
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
        if (!File.Exists(gmlFilePath))
        {
            UnityEngine.Debug.LogError($"CityGMLImporter: File not found: {gmlFilePath}");
            return;
        }

        lineMat = new Material(Shader.Find("Hidden/Internal-Colored"));
        lineMat.hideFlags = HideFlags.HideAndDontSave;
        lineMat.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
        lineMat.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
        lineMat.SetInt("_Cull", (int)UnityEngine.Rendering.CullMode.Off);
        lineMat.SetInt("_ZWrite", 0);

        StartCoroutine(LoadGML());
    }

    void Update()
    {

    }

    IEnumerator LoadGML()
    {
        Stopwatch sw = Stopwatch.StartNew();

        XmlDocument doc = new XmlDocument();
        doc.Load(gmlFilePath);

        XmlNodeList buildings = doc.SelectNodes("//*[local-name()='Building']");
        if (buildings == null || buildings.Count == 0)
        {
            UnityEngine.Debug.LogError("CityGMLImporter: No buildings found.");
            yield break;
        }

        UnityEngine.Debug.Log($"CityGMLImporter: Found {buildings.Count} buildings.");

        HashSet<string> sourceFields = new HashSet<string>();
        HashSet<string> retainedFields = new HashSet<string>();

        XmlNode firstBuilding = buildings[0];
        foreach (XmlNode attr in firstBuilding.SelectNodes("*[local-name()='stringAttribute']"))
        {
            string n = attr.Attributes?["name"]?.Value ?? "";
            if (!string.IsNullOrEmpty(n)) sourceFields.Add(n);
        }
        if (firstBuilding.SelectSingleNode("*[local-name()='name']") != null)
            sourceFields.Add("gml:name");

        attributeFieldsInSource = sourceFields.Count > 0
            ? string.Join(", ", sourceFields)
            : "None found";

        int buildingIndex = 0;
        foreach (XmlNode building in buildings)
        {
            string gmlId = building.Attributes?["gml:id"]?.Value ?? $"Building_{buildingIndex}";
            string lotName = building.SelectSingleNode("*[local-name()='name']")?.InnerText.Trim() ?? gmlId;

            Dictionary<string, string> attributes = new Dictionary<string, string>();
            foreach (XmlNode attr in building.SelectNodes("*[local-name()='stringAttribute']"))
            {
                string attrName = attr.Attributes?["name"]?.Value ?? "";
                string attrValue = attr.SelectSingleNode("*[local-name()='stringVal']")?.InnerText.Trim() ?? "";
                if (!string.IsNullOrEmpty(attrName))
                {
                    attributes[attrName] = attrValue;
                    retainedFields.Add(attrName);
                }
            }
            retainedFields.Add("gml:name");

            GameObject buildingGO = new GameObject(lotName);
            buildingGO.transform.SetParent(transform);

            CityGMLMetadata meta = buildingGO.AddComponent<CityGMLMetadata>();
            meta.gmlId = gmlId;
            meta.lotName = lotName;
            meta.attributes = attributes;

            XmlNodeList posLists = building.SelectNodes(".//*[local-name()='posList']");
            int polyIndex = 0;
            foreach (XmlNode posListNode in posLists)
            {
                List<Vector3> verts = ParsePosList(posListNode.InnerText.Trim());
                if (verts == null || verts.Count < 3) continue;

                if (verts.Count > 3 && Vector3.Distance(verts[0], verts[verts.Count - 1]) < 0.001f)
                    verts.RemoveAt(verts.Count - 1);
                if (verts.Count < 3) continue;

                for (int i = 0; i < verts.Count; i++)
                {
                    Vector3 a = buildingGO.transform.TransformPoint(verts[i]);
                    Vector3 b = buildingGO.transform.TransformPoint(verts[(i + 1) % verts.Count]);
                    allEdges.Add(new Vector3[] { a, b });
                }

                List<int> triangles = new List<int>();
                for (int i = 1; i < verts.Count - 1; i++)
                {
                    triangles.Add(0); triangles.Add(i); triangles.Add(i + 1);
                }

                Mesh mesh = new Mesh();
                mesh.name = $"{gmlId}_poly{polyIndex}";
                mesh.vertices = verts.ToArray();
                mesh.triangles = triangles.ToArray();
                mesh.RecalculateNormals();
                mesh.RecalculateBounds();

                GameObject polyGO = new GameObject($"Polygon_{polyIndex}");
                polyGO.transform.SetParent(buildingGO.transform);
                polyGO.AddComponent<MeshFilter>().sharedMesh = mesh;
                var mr = polyGO.AddComponent<MeshRenderer>();
                var mat = new Material(Shader.Find("Hidden/Internal-Colored"));
                mat.color = Color.clear;
                mr.sharedMaterial = mat;

                totalVertices += verts.Count;
                totalFaces += triangles.Count / 3;
                polyIndex++;
            }

            buildingIndex++;
            if (buildingIndex % 10 == 0)
            {
                UnityEngine.Debug.Log($"CityGMLImporter: Processed {buildingIndex} buildings...");
                yield return null;
            }
        }

        sw.Stop();
        importTimeSeconds = (float)sw.Elapsed.TotalSeconds;
        attributeFieldsRetained = retainedFields.Count > 0 ? string.Join(", ", retainedFields) : "None";

        loadComplete = true;
        UnityEngine.Debug.Log("!Load Complete!");
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

    List<Vector3> ParsePosList(string posText)
    {
        string[] parts = posText.Split(
            new char[] { ' ', '\t', '\n', '\r' },
            System.StringSplitOptions.RemoveEmptyEntries);

        if (parts.Length < 9) return null;

        List<Vector3> verts = new List<Vector3>();
        for (int i = 0; i + 2 < parts.Length; i += 3)
        {
            if (!double.TryParse(parts[i], System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture, out double lon)) continue;
            if (!double.TryParse(parts[i + 1], System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture, out double lat)) continue;
            if (!double.TryParse(parts[i + 2], System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture, out double alt)) continue;

            float x = (float)((lon - originLon) * MetresPerDegLon);
            float y = (float)(alt - originAlt);
            float z = (float)((lat - originLat) * MetresPerDegLat);
            verts.Add(new Vector3(x, y, z));
        }
        return verts;
    }

}

public class CityGMLMetadata : MonoBehaviour
{
    public string gmlId;
    public string lotName;
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
