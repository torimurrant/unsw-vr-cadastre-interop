

#pragma once

#include "ProceduralMeshComponent.h"

#include "CoreMinimal.h"

class CITYGMLIMPORTEREDITOR_API PolygonConstructor
{

	
public:	
	PolygonConstructor();
	
	
	TArray<FVector> Polygon;
	FVector GenerateMesh(int& SectionNum,UProceduralMeshComponent* MeshProc, UStaticMeshComponent* Mesh);
	FProcMeshTangent ResetMesh();
	FVector RegulationVector;
	FVector RelativeLocation;
	bool isBack = false;
	FVector LineIntersection;
	bool isfirsttime;
	double tolerance = 0.0005;
	double breaks;
	double Polygons;
	
	TArray<FVector>Vertices;
	TArray<FVector2D>TwoDVertices;
	TArray<int32>Triangles;
	TArray<FVector>Normals;
	TArray<FProcMeshTangent>Tangents;
	TArray<FVector2D> UVs;
	TArray<FLinearColor>Colors;
	void AddPolygon(TArray<FVector> RawVertices, int32& TriIndex);
	UStaticMesh* ConvertToStaticMesh(UProceduralMeshComponent* ProcMeshComp, FString Filename, int BuildingNumber);
private:
	int16 scaleValue = 100;
	bool DoesIntersect(FVector EdgeStart, FVector Edge,FVector NormalizedEdge, FVector NewLineStart, FVector NewLine, FVector NormalizedNewLine, double paraleluppervalue);
	bool PointInside(TArray<FVector> RawVertices, TArray<FVector> Edges, TArray<FVector> NormalizedEdges,FVector PointonNewLine);
	bool Verticesto2D(TArray<FVector> RawVertices);
	bool IsOnePlane(TArray<FVector> RawVertices);
	

};
