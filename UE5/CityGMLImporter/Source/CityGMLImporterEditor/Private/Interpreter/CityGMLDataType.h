#pragma once
#include "ProceduralMeshComponent.h"

#include "Editor.h"
#include "FastXml.h"
#include "CoreMinimal.h"
#include "CityGMLDataType.generated.h"

USTRUCT()
struct FPolygon
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FVector> ArrayOfVertices;

	FPolygon()
	{
		
	}
};



UCLASS()
class CITYGMLIMPORTEREDITOR_API ACityGMLDataType : public AActor
{
	GENERATED_BODY()
protected:
	virtual void PostActorCreated() override;
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
public:
	ACityGMLDataType();
	UPROPERTY(VisibleAnywhere, Category = "Scene")
	USceneComponent* ThisScene;
	UPROPERTY(VisibleAnywhere, Category = "Mesh")
	UProceduralMeshComponent* ThisMeshProc;
	UPROPERTY(VisibleAnywhere, Category = "Mesh")
	UStaticMeshComponent* ThisMesh;
	UPROPERTY()
	bool gotAddress;
	
	static FPolygon ProcessVerticesToPolygon(FString Coordinates);
	static TArray<FVector> ProcessStringToVertices(FString RawData);

	UPROPERTY()
	TArray<FPolygon> Polygons;

	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString Function = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString Usage = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString YearOfConstruction = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString YearOfDemolition = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString RoofType = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString MeasuredHeight = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString StoreysAboveGround = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString StoreysBelowGround = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString StoreysHeightAboveGround = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString StoreysHeightBelowGround = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString Address = "";
		
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString LOD = "";
	
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString CountryName;
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString LocalityName;
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString ThoroughfareName;
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString ThoroughfareNumber;
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString DepartmentName;
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString DependentLocalityName;
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString DependentLocalityNumber;
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString PostalCodeNumber;
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	FString PostalCodeNumberExtension;
	
	UPROPERTY(BlueprintReadWrite,VisibleAnywhere, Category = "Building Data")
	TArray<FString> UnusedData;
	
};

