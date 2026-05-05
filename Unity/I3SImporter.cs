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
    [Tooltip("AGOL API key")]
    public string apiKey = "ADD API KEY HERE";
    [Tooltip("I3S REST URL")]
    public string layerSource = "ADD URL HERE";
    [Tooltip("Layer opacity (0-1)")]
    [Range(0f, 1f)]
    public float layerOpacity = 1.0f;

    [Header("Geometry")]
    [Tooltip("Total vertices across all loaded mesh primitives")]
    public int totalVertices = 0;
    [Tooltip("Total triangulated faces")]
    public int totalFaces = 0;
    [Tooltip("Mesh primitives loaded")]
    public int meshPrimitivesFound = 0;

    [Header("Performance")]
    [Tooltip("Import time (seconds)")]
    public float importTimeSeconds = 0f;

    private ArcGISMapComponent mapComponent;
    private bool loadComplete = false;
    private float loadStartTime = 0f;

    // origin point matching study area
    private double originLatitude = -32.92564;
    private double originLongitude = 151.76675;
    private double originAltitude = 0.0;

    void Start()
    {
        loadStartTime = Time.realtimeSinceStartup;
        SetupMap();
    }

    void Update()
    {
    
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
        var sceneLayer = new ArcGIS3DObjectSceneLayer(layerSource, layerOpacity, true, apiKey);
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
            var cam = camGO.AddComponent<Camera>();
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

        }

        importTimeSeconds = Time.realtimeSinceStartup - loadStartTime;
        RunGeometryCount();

        loadComplete = true;
        UnityEngine.Debug.Log("!Load complete!");
        UnityEngine.Debug.Log($"Import time: {importTimeSeconds:F2}s");
        UnityEngine.Debug.Log($"Total vertices: {totalVertices}");
        UnityEngine.Debug.Log($"Total faces: {totalFaces}");
        UnityEngine.Debug.Log($"Mesh primitives: {meshPrimitivesFound}");
        UnityEngine.Debug.Log("!=======================!");
    }

    int CountMeshFilters()
    {
        int count = 0;
        if (mapComponent != null)
            foreach (MeshFilter mf in mapComponent.GetComponentsInChildren<MeshFilter>(true))
                if (mf.sharedMesh != null) count++;
        if (count == 0)
            foreach (MeshFilter mf in FindObjectsByType<MeshFilter>(FindObjectsSortMode.None))
                if (mf.sharedMesh != null) count++;
        return count;
    }

    void RunGeometryCount()
    {
        totalVertices = 0;
        totalFaces = 0;
        meshPrimitivesFound = 0;

        MeshFilter[] meshFilters = mapComponent != null
            ? mapComponent.GetComponentsInChildren<MeshFilter>(true)
            : FindObjectsByType<MeshFilter>(FindObjectsSortMode.None);

        foreach (MeshFilter mf in meshFilters)
        {
            if (mf.sharedMesh == null) continue;
            meshPrimitivesFound++;
            totalVertices += mf.sharedMesh.vertexCount;
            totalFaces += mf.sharedMesh.triangles.Length / 3;
        }
    }
}
