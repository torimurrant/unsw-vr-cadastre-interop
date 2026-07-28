#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "KMZActor.generated.h"

UCLASS()
class KMZIMPORTER_API AKMZActor : public AActor
{
	GENERATED_BODY()

public:
	AKMZActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMZ Attributes")
	FString UniqueId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMZ Attributes")
	FString PlanNumber;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMZ Attributes")
	int32 LotNumber;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMZ Attributes")
	FString Type;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMZ Attributes")
	FString Label;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMZ Attributes")
	int32 Entitlement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMZ Geometry")
	UProceduralMeshComponent* MeshComponent;

	void BuildMesh(const TArray<FVector>& Vertices,
	               const TArray<int32>& Triangles,
	               const TArray<FVector>& Normals);
};
