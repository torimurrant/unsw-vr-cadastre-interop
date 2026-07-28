#pragma once

#include "CoreMinimal.h"

struct FDAEMesh
{
	TArray<FVector> Vertices;
	TArray<int32>   Triangles;
	TArray<FVector> Normals;
};

class FDAEParser
{
public:
	static bool Parse(const FString& FilePath, FDAEMesh& OutMesh);

private:
	static TArray<float> ParseFloatArray(const FString& Text);

	static TArray<int32> ParseIntArray(const FString& Text);
};
