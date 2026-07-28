#include "CityGMLDataType.h"

#include "CoreMinimal.h"
#include "Net/Core/Connection/NetConnectionFaultRecoveryBase.h"

ACityGMLDataType::ACityGMLDataType()
{	
	ThisScene = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = ThisScene;

	ThisMeshProc = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeshProc"));
	ThisMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	ThisMesh->SetupAttachment(RootComponent);
	
}

void ACityGMLDataType::PostActorCreated()
{
	Super::PostActorCreated();
}

void ACityGMLDataType::PostLoad()
{
	Super::PostLoad();
}

void ACityGMLDataType::BeginPlay()
{
	Super::BeginPlay();
}

FPolygon ACityGMLDataType::ProcessVerticesToPolygon(FString Coordinates)
{
	FPolygon newPolygon;
	newPolygon.ArrayOfVertices = ProcessStringToVertices(Coordinates);
	return newPolygon;
}

TArray<FVector> ACityGMLDataType::ProcessStringToVertices(FString RawData)
{
	TArray<FVector> Polygon;
	TArray<double> doublevertices;
	FString stringvertices;
	for (int i = 0; i < RawData.GetCharArray().Num(); i++)
	{
		if (RawData.GetCharArray()[i]!=L' ') {
			stringvertices += RawData.GetCharArray()[i];
		}
		else
		{
			doublevertices.Add(FCString::Atod(*stringvertices)*100);
			stringvertices.Reset();
		}
	}
	doublevertices.Add(FCString::Atod(*stringvertices)*100);
	stringvertices.Reset();
	
	for (int i = 0; (3*i) < doublevertices.Num(); i++)
	{
		Polygon.Add(FVector(doublevertices[3*i+1], doublevertices[3*i], doublevertices[3*i+2]));
	}
	return Polygon;
}
