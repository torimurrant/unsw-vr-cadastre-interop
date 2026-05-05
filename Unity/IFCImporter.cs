using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.Globalization;

public class IFCImporter : MonoBehaviour
{
    [Header("File")]
    [Tooltip("File path")]
    public string ifcFilePath = @"ADD FILE PATH HERE";
    [Header("Wireframe colour")]
    public Color wireColour = new Color(0.2f, 1.0f, 0.5f, 1.0f);

    [Header("Geometry")]
    [Tooltip("Total vertices")]
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

    private Dictionary<string, string[]> E = new Dictionary<string, string[]>();

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

        StartCoroutine(LoadIFC());
    }

    void Update()
    {

    }

    IEnumerator LoadIFC()
    {
        Stopwatch sw = Stopwatch.StartNew();

        string content = File.ReadAllText(ifcFilePath);
        yield return null;

        foreach (Match m in Regex.Matches(content, @"#(\d+)=([A-Z]+)\((.+?)\);\r?\n", RegexOptions.Singleline))
            E[m.Groups[1].Value] = new[] { m.Groups[2].Value, m.Groups[3].Value };

        UnityEngine.Debug.Log($"IFCImporter: Parsed {E.Count} entities.");
        yield return null;

        HashSet<string> sourceFields = new HashSet<string>();
        HashSet<string> retainedFields = new HashSet<string>();

        foreach (var kv in E)
            if (kv.Value[0] == "IFCPROPERTYSINGLEVALUE")
            {
                var nm = Regex.Match(kv.Value[1], @"'([^']*)'");
                if (nm.Success) sourceFields.Add(nm.Groups[1].Value);
            }
        sourceFields.Add("Name");
        sourceFields.Add("GlobalId");

        attributeFieldsInSource = sourceFields.Count > 0
            ? string.Join(", ", sourceFields)
            : "None found";

        Dictionary<string, Dictionary<string, string>> elemProps = BuildElemProps();

        Dictionary<string, string> storeyIdName = new Dictionary<string, string>();
        foreach (var kv in E)
            if (kv.Value[0] == "IFCBUILDINGSTOREY")
            {
                string n = GetArg(kv.Value[1], 2);
                storeyIdName[kv.Key] = Unquote(n);
            }

        Dictionary<string, string> elemStorey = new Dictionary<string, string>();
        foreach (var kv in E)
        {
            if (kv.Value[0] != "IFCRELCONTAINEDINSPATIALSTRUCTURE") continue;
            var refs = Refs(kv.Value[1]);
            if (refs.Count < 2) continue;
            string sid = refs[refs.Count - 1];
            if (!storeyIdName.ContainsKey(sid)) continue;
            string sname = storeyIdName[sid];
            for (int i = 0; i < refs.Count - 1; i++) elemStorey[refs[i]] = sname;
        }

        Dictionary<string, List<string>> proxyToExtrusions = BuildProxyExtrusionMap();

        Dictionary<string, GameObject> storeyGOs = new Dictionary<string, GameObject>();
        foreach (string s in storeyIdName.Values)
            if (!storeyGOs.ContainsKey(s))
            { var g = new GameObject(s); g.transform.SetParent(transform); storeyGOs[s] = g; }

        var unassignedGO = new GameObject("Unassigned");
        unassignedGO.transform.SetParent(transform);

        int elemIdx = 0;
        foreach (var kv in E)
        {
            if (kv.Value[0] != "IFCBUILDINGELEMENTPROXY") continue;

            string proxyId = kv.Key;
            string rawName = Unquote(GetArg(kv.Value[1], 2));
            string lotName = rawName.Contains(":") ? rawName.Split(':')[0] : rawName;
            if (string.IsNullOrEmpty(lotName)) lotName = $"Element_{elemIdx}";

            string storeyName = elemStorey.ContainsKey(proxyId) ? elemStorey[proxyId] : "Unassigned";
            GameObject parentGO = storeyGOs.ContainsKey(storeyName) ? storeyGOs[storeyName] : unassignedGO;

            var props = elemProps.ContainsKey(proxyId) ? elemProps[proxyId] : new Dictionary<string, string>();

            if (!proxyToExtrusions.ContainsKey(proxyId)) { elemIdx++; continue; }
            List<string> extIds = proxyToExtrusions[proxyId];
            if (extIds.Count == 0) { elemIdx++; continue; }

            GameObject elemGO = new GameObject(lotName);
            elemGO.transform.SetParent(parentGO.transform);

            IFCMetadata meta = elemGO.AddComponent<IFCMetadata>();
            meta.ifcId = proxyId;
            meta.lotName = lotName;
            meta.storeyName = storeyName;
            meta.properties = props;

            foreach (string k in props.Keys) retainedFields.Add(k);
            retainedFields.Add("Name");
            retainedFields.Add("GlobalId");

            int meshIdx = 0;
            foreach (string extId in extIds)
            {
                var extData = E[extId][1];
                var extRefs = Refs(extData);
                if (extRefs.Count < 1) continue;

                var depthM = Regex.Match(extData, @",([-\d.E+]+)$");
                if (!depthM.Success) continue;
                if (!float.TryParse(depthM.Groups[1].Value, NumberStyles.Float,
                    CultureInfo.InvariantCulture, out float depth)) continue;

                string profileRef = extRefs[0];
                List<Vector2> profile = GetProfile(profileRef);
                if (profile == null || profile.Count < 3) continue;

                GameObject meshGO = new GameObject($"Extrusion_{meshIdx}");
                meshGO.transform.SetParent(elemGO.transform);
                BuildMesh(meshGO, profile, depth);
                meshIdx++;
            }

            elemIdx++;
            if (elemIdx % 20 == 0)
            {
                UnityEngine.Debug.Log($"IFCImporter: {elemIdx} elements processed...");
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

    Dictionary<string, List<string>> BuildProxyExtrusionMap()
    {
        var result = new Dictionary<string, List<string>>();
        foreach (var kv in E)
        {
            if (kv.Value[0] != "IFCBUILDINGELEMENTPROXY") continue;
            string proxyId = kv.Key;
            List<string> exts = new List<string>();

            foreach (string r1 in Refs(kv.Value[1]))
            {
                if (!E.ContainsKey(r1) || E[r1][0] != "IFCPRODUCTDEFINITIONSHAPE") continue;
                foreach (string r2 in Refs(E[r1][1]))
                {
                    if (!E.ContainsKey(r2) || E[r2][0] != "IFCSHAPEREPRESENTATION") continue;
                    foreach (string r3 in Refs(E[r2][1]))
                    {
                        if (!E.ContainsKey(r3) || E[r3][0] != "IFCMAPPEDITEM") continue;
                        var mapRefs = Refs(E[r3][1]);
                        if (mapRefs.Count == 0) continue;
                        string repMapId = mapRefs[0];
                        if (!E.ContainsKey(repMapId) || E[repMapId][0] != "IFCREPRESENTATIONMAP") continue;
                        var repRefs = Refs(E[repMapId][1]);
                        if (repRefs.Count < 2) continue;
                        string shapeRepId = repRefs[1];
                        if (!E.ContainsKey(shapeRepId) || E[shapeRepId][0] != "IFCSHAPEREPRESENTATION") continue;
                        foreach (string r4 in Refs(E[shapeRepId][1]))
                            if (E.ContainsKey(r4) && E[r4][0] == "IFCEXTRUDEDAREASOLID") exts.Add(r4);
                    }
                }
            }
            if (exts.Count > 0) result[proxyId] = exts;
        }
        return result;
    }

    List<Vector2> GetProfile(string profileId)
    {
        if (!E.ContainsKey(profileId) || E[profileId][0] != "IFCARBITRARYCLOSEDPROFILEDEF") return null;
        var pRefs = Refs(E[profileId][1]);
        if (pRefs.Count == 0) return null;
        string curveId = pRefs[pRefs.Count - 1];
        if (!E.ContainsKey(curveId) || E[curveId][0] != "IFCCOMPOSITECURVE") return null;

        string curveData = E[curveId][1];
        var segPart = Regex.Match(curveData, @"^\((#\d+(?:,#\d+)*)\)");
        if (!segPart.Success) return null;

        List<Vector2> pts = new List<Vector2>();
        foreach (string segRef in Refs(segPart.Groups[1].Value))
        {
            if (!E.ContainsKey(segRef) || E[segRef][0] != "IFCCOMPOSITECURVESEGMENT") continue;
            var segRefs2 = Refs(E[segRef][1]);
            if (segRefs2.Count == 0) continue;
            string geoId = segRefs2[segRefs2.Count - 1];
            if (!E.ContainsKey(geoId)) continue;

            string gtype = E[geoId][0];
            if (gtype == "IFCPOLYLINE")
            {
                var ptRefs = Refs(E[geoId][1]);
                if (ptRefs.Count > 0) { var pt = GetPoint2D(ptRefs[0]); if (pt.HasValue) pts.Add(pt.Value); }
            }
            else if (gtype == "IFCTRIMMEDCURVE")
            {
                foreach (string tr in Refs(E[geoId][1]))
                {
                    if (!E.ContainsKey(tr) || E[tr][0] != "IFCCARTESIANPOINT") continue;
                    var pt = GetPoint2D(tr);
                    if (pt.HasValue) { pts.Add(pt.Value); break; }
                }
            }
            else if (gtype == "IFCCIRCLE")
            {
                var circPts = ApproxCircle(geoId);
                if (circPts != null) pts.AddRange(circPts);
            }
        }
        return pts.Count >= 3 ? pts : null;
    }

    Vector2? GetPoint2D(string id)
    {
        if (!E.ContainsKey(id) || E[id][0] != "IFCCARTESIANPOINT") return null;
        var nums = Regex.Matches(E[id][1], @"-?[\d.E+\-]+");
        if (nums.Count < 2) return null;
        if (!float.TryParse(nums[0].Value, NumberStyles.Float, CultureInfo.InvariantCulture, out float x)) return null;
        if (!float.TryParse(nums[1].Value, NumberStyles.Float, CultureInfo.InvariantCulture, out float y)) return null;
        return new Vector2(x, y);
    }

    List<Vector2> ApproxCircle(string circleId)
    {
        var radiusM = Regex.Match(E[circleId][1], @",([\d.E+\-]+)\)$");
        if (!radiusM.Success) return null;
        if (!float.TryParse(radiusM.Groups[1].Value, NumberStyles.Float, CultureInfo.InvariantCulture, out float r)) return null;
        var pts = new List<Vector2>();
        for (int i = 0; i < 8; i++)
        {
            float a = i * 2f * Mathf.PI / 8f;
            pts.Add(new Vector2(r * Mathf.Cos(a), r * Mathf.Sin(a)));
        }
        return pts;
    }

    void BuildMesh(GameObject go, List<Vector2> profile, float height)
    {
        int n = profile.Count;
        var verts = new List<Vector3>();
        for (int i = 0; i < n; i++) verts.Add(new Vector3(profile[i].x, 0, profile[i].y));
        for (int i = 0; i < n; i++) verts.Add(new Vector3(profile[i].x, height, profile[i].y));

        var tris = new List<int>();
        for (int i = 1; i < n - 1; i++) { tris.Add(0); tris.Add(i + 1); tris.Add(i); }
        for (int i = 1; i < n - 1; i++) { tris.Add(n); tris.Add(n + i); tris.Add(n + i + 1); }
        for (int i = 0; i < n; i++)
        {
            int nx = (i + 1) % n;
            tris.Add(i); tris.Add(n + i); tris.Add(nx);
            tris.Add(nx); tris.Add(n + i); tris.Add(n + nx);
        }

        totalVertices += verts.Count;
        totalFaces += tris.Count / 3;

        Mesh mesh = new Mesh();
        mesh.vertices = verts.ToArray();
        mesh.triangles = tris.ToArray();
        mesh.RecalculateNormals();
        mesh.RecalculateBounds();

        go.AddComponent<MeshFilter>().sharedMesh = mesh;
        var mr = go.AddComponent<MeshRenderer>();
        var mat = new Material(Shader.Find("Hidden/Internal-Colored"));
        mat.color = Color.clear;
        mr.sharedMaterial = mat;

        for (int i = 0; i < n; i++)
        {
            int nx = (i + 1) % n;
            allEdges.Add(new[] { go.transform.TransformPoint(verts[i]), go.transform.TransformPoint(verts[nx]) });
            allEdges.Add(new[] { go.transform.TransformPoint(verts[n + i]), go.transform.TransformPoint(verts[n + nx]) });
            allEdges.Add(new[] { go.transform.TransformPoint(verts[i]), go.transform.TransformPoint(verts[n + i]) });
        }
    }

    Dictionary<string, Dictionary<string, string>> BuildElemProps()
    {
        var psetData = new Dictionary<string, Dictionary<string, string>>();
        foreach (var kv in E)
        {
            if (kv.Value[0] != "IFCPROPERTYSET") continue;
            var pd = new Dictionary<string, string>();
            foreach (string pr in Refs(kv.Value[1]))
            {
                if (!E.ContainsKey(pr) || E[pr][0] != "IFCPROPERTYSINGLEVALUE") continue;
                var nm = Regex.Match(E[pr][1], @"'([^']*)'");
                var vm = Regex.Match(E[pr][1], @"IFC\w+\('([^']*)'\)");
                if (nm.Success) pd[nm.Groups[1].Value] = vm.Success ? vm.Groups[1].Value : "";
            }
            psetData[kv.Key] = pd;
        }

        var result = new Dictionary<string, Dictionary<string, string>>();
        foreach (var kv in E)
        {
            if (kv.Value[0] != "IFCRELDEFINESBYPROPERTIES") continue;
            var refs = Refs(kv.Value[1]);
            if (refs.Count < 2) continue;
            string psRef = refs[refs.Count - 1];
            if (!psetData.ContainsKey(psRef)) continue;
            for (int i = 0; i < refs.Count - 1; i++)
            {
                if (!result.ContainsKey(refs[i])) result[refs[i]] = new Dictionary<string, string>();
                foreach (var p in psetData[psRef]) result[refs[i]][p.Key] = p.Value;
            }
        }
        return result;
    }

    void OnRenderObject()
    {
        if (allEdges == null || allEdges.Count == 0 || lineMat == null) return;
        lineMat.SetPass(0);
        GL.Begin(GL.LINES);
        GL.Color(wireColour);
        foreach (var e in allEdges) { GL.Vertex(e[0]); GL.Vertex(e[1]); }
        GL.End();
    }

    List<string> Refs(string data)
    {
        var result = new List<string>();
        foreach (Match m in Regex.Matches(data, @"#(\d+)")) result.Add(m.Groups[1].Value);
        return result;
    }

    string GetArg(string data, int index)
    {
        int depth = 0, start = 0, count = 0;
        for (int i = 0; i < data.Length; i++)
        {
            if (data[i] == '(') depth++;
            else if (data[i] == ')') depth--;
            else if (data[i] == '\'') { int j = i + 1; while (j < data.Length && data[j] != '\'') j++; i = j; }
            else if (data[i] == ',' && depth == 0)
            {
                if (count == index) return data.Substring(start, i - start).Trim();
                count++; start = i + 1;
            }
        }
        if (count == index) return data.Substring(start).Trim();
        return "";
    }

    string Unquote(string s)
    {
        s = s.Trim();
        if (s.StartsWith("'") && s.EndsWith("'")) return s.Substring(1, s.Length - 2);
        return s == "$" ? "" : s;
    }

}

public class IFCMetadata : MonoBehaviour
{
    public string ifcId;
    public string lotName;
    public string storeyName;
    public List<string> propertyKeys = new List<string>();
    public List<string> propertyValues = new List<string>();

    private Dictionary<string, string> _properties;
    public Dictionary<string, string> properties
    {
        get { return _properties; }
        set
        {
            _properties = value;
            propertyKeys.Clear();
            propertyValues.Clear();
            if (value != null)
                foreach (var kvp in value) { propertyKeys.Add(kvp.Key); propertyValues.Add(kvp.Value); }
        }
    }
}
