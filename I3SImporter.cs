using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using Esri.ArcGISMapsSDK.Components;
using Esri.ArcGISMapsSDK.Utils.GeoCoord;
using Esri.GameEngine.Map;
using Esri.GameEngine.Layers;
using Esri.GameEngine.Geometry;
using Esri.Unity;
using Esri.GameEngine.Elevation;

public class I3SImporter : MonoBehaviour
{
    [Header("File")]
    [Tooltip("ArcGIS Online API Key")]
    public string apiKey = "ADD API KEY HERE";
    [Tooltip("REST Endpoint URL")]
    public string layerSource = "ADD URL HERE";
    [Tooltip(Layer Opacity (0-1).")]f
    [Range(0f, 1f)]
    public float layerOpacity = 1.0f;

    [Header("Geometry")]
    [Tooltip("Total vertices")]
    public int totalVertices = 0;
    [Tooltip("Total triangulated faces")]
    public int totalFaces = 0;
    [Tooltip("Mesh primitives loaded")]
    public int meshPrimitivesFound = 0;
    [Tooltip("Degenerate faces detected")]
    public int degenerateFaces = 0;
    [Tooltip("Geometric errors detected")]
    public string geometricErrors = "None";

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

    private ArcGISMapComponent mapComponent;
    private bool loadComplete = false;
    private float loadStartTime = 0f;
    private float fpsTimer = 0f;
    private int fpsFrameCount = 0;

    void Start()
    {
        baselineRAM_MB = GetRAM_MB();
        loadStartTime = Time.realtimeSinceStartup;
        SetupMap();
    }

    void SetupMap()
    {
        mapComponent = FindFirstObjectByType<ArcGISMapComponent>();
        if (mapComponent == null)
        {
            GameObject mapGO = new GameObject("ArcGISMap");
            mapComponent = mapGO.AddComponent<ArcGISMapComponent>();
        }

        mapComponent.APIKey = apiKey;
        mapComponent.MapType = ArcGISMapType.Local;
        mapComponent.OriginPosition = new ArcGISPoint(
            originLongitude, originLatitude, originAltitude,
            ArcGISSpatialReference.WGS84());

        var arcGISMap = new ArcGISMap(ArcGISMapType.Local);
        if (addBasemap)
        {
            arcGISMap.Basemap = new ArcGISBasemap(
                ArcGISBasemapStyle.ArcGISImagery, apiKey);
        }
        if (addElevation)
        {
            arcGISMap.Elevation = new ArcGISMapElevation(
                new ArcGISImageElevationSource(
                    "https://elevation3d.arcgis.com/arcgis/rest/services/WorldElevation3D/Terrain3D/ImageServer",
                    "Terrain3D", apiKey));
        }

        var sceneLayer = new ArcGIS3DObjectSceneLayer(
            layerSource, layerOpacity, true, apiKey);
        arcGISMap.Layers.Add(sceneLayer);
        mapComponent.Map = arcGISMap;
        Camera mainCam = Camera.main;
        if (mainCam != null)
        {
            mainCam.transform.SetParent(mapComponent.transform);

            if (mainCam.GetComponent<ArcGISCameraComponent>() == null)
                mainCam.gameObject.AddComponent<ArcGISCameraComponent>();

            mainCam.transform.localPosition = new Vector3(0, 500, -200);
            mainCam.transform.rotation = Quaternion.Euler(45f, 0f, 0f);
        }
        else
        {
            GameObject camGO = new GameObject("ArcGISCamera");
            camGO.transform.SetParent(mapComponent.transform);
            camGO.transform.localPosition = new Vector3(0, 500, -200);
            camGO.transform.rotation = Quaternion.Euler(45f, 0f, 0f);
            Camera cam = camGO.AddComponent<Camera>();
            cam.tag = "MainCamera";
            camGO.AddComponent<ArcGISCameraComponent>();
        }

        StartCoroutine(MonitorLoad());
    }

    IEnumerator MonitorLoad()
    {
        yield return new WaitForSeconds(2f);

        int previousCount = 0;
        int stableFrames = 0;
        float waitedFor = 0f;
        float maxWait = 60f;

        while (stableFrames < 60 && waitedFor < maxWait)
        {
            yield return new WaitForSeconds(0.5f);
            waitedFor += 0.5f;

            int current = CountMeshFilters();

            if (current > 0 && current == previousCount)
                stableFrames++;
            else
                stableFrames = 0;

            previousCount = current;

            if (current > 0 && waitedFor % 5f < 0.6f)
                UnityEngine.Debug.Log($"Tiles loading... {current} mesh primitives, stable for {stableFrames}");
        }

        importTimeSeconds = Time.realtimeSinceStartup - loadStartTime;
        RunGeometryCount();
        loadComplete = true;

        UnityEngine.Debug.Log("!Load Complete!");
        UnityEngine.Debug.Log($"Import time:     {importTimeSeconds:F2}s");
        UnityEngine.Debug.Log($"Total vertices:  {totalVertices}");
        UnityEngine.Debug.Log($"Total faces:     {totalFaces}");
        UnityEngine.Debug.Log($"Mesh primitives: {meshPrimitivesFound}");
        UnityEngine.Debug.Log("!=======================!");
    }

    int CountMeshFilters()
    {
        int count = 0;
        if (mapComponent != null)
        {
            foreach (MeshFilter mf in mapComponent.GetComponentsInChildren<MeshFilter>(true))
                if (mf.sharedMesh != null) count++;
        }
        if (count == 0)
        {
            foreach (MeshFilter mf in FindObjectsByType<MeshFilter>(FindObjectsSortMode.None))
                if (mf.sharedMesh != null) count++;
        }
        return count;
    }

    void RunGeometryCount()
    {
        totalVertices = 0;
        totalFaces = 0;
        degenerateFaces = 0;
        meshPrimitivesFound = 0;

        var materials = new HashSet<Material>();

        MeshFilter[] meshFilters = mapComponent != null
            ? mapComponent.GetComponentsInChildren<MeshFilter>(true)
            : FindObjectsByType<MeshFilter>(FindObjectsSortMode.None);

        foreach (MeshFilter mf in meshFilters)
        {
            if (mf.sharedMesh == null) continue;
            meshPrimitivesFound++;
            totalVertices += mf.sharedMesh.vertexCount;
            totalFaces += mf.sharedMesh.triangles.Length / 3;

            int[] tris = mf.sharedMesh.triangles;
            for (int i = 0; i < tris.Length; i += 3)
                if (tris[i] == tris[i + 1] || tris[i + 1] == tris[i + 2] || tris[i] == tris[i + 2])
                    degenerateFaces++;

            var mr = mf.GetComponent<MeshRenderer>();
            if (mr != null && mr.sharedMaterial != null)
                materials.Add(mr.sharedMaterial);
        }

        if (degenerateFaces > 0)
            geometricErrors = $"{degenerateFaces} degenerate faces detected";

    void Update()
    {
        if (!loadComplete) return;

        fpsTimer += Time.unscaledDeltaTime;
        fpsFrameCount++;
        if (fpsTimer >= 0.5f)
        { currentFPS = fpsFrameCount / fpsTimer; fpsTimer = 0f; fpsFrameCount = 0; }

        ramUsageMB = GetRAM_MB();
    }
}
