#include "KMZActor.h"

AKMZActor::AKMZActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("KMZMesh"));
	RootComponent = MeshComponent;

	LotNumber   = 0;
	Entitlement = 0;
}

void AKMZActor::BuildMesh(const TArray<FVector>& Vertices,
                           const TArray<int32>&   Triangles,
                           const TArray<FVector>& Normals)
{
	if (Vertices.Num() == 0 || Triangles.Num() == 0)
	{
		return;
	}

	TArray<FVector2D> UV0;
	TArray<FColor>    VertexColors;
	TArray<FProcMeshTangent> Tangents;

	MeshComponent->CreateMeshSection(
		0,
		Vertices,
		Triangles,
		Normals,
		UV0,
		VertexColors,
		Tangents,
		true
	);
}
