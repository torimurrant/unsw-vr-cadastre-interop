#pragma once
#include "CityGMLDataType.h"
#include "PolygonConstructor.h"

#include "FastXml.h"
#include "CoreMinimal.h"

UENUM(BlueprintType)
enum EMeshSection
{
	NOTSPECIFIED = 0 UMETA(DisplayName = "NOTSPECIFIED"),
	GROUND		 = 1 UMETA(DisplayName = "GROUND"),
	WALL		 = 2 UMETA(DisplayName = "WALL"),
	ROOF		 = 3 UMETA(DisplayName = "ROOF")
};

class CITYGMLIMPORTEREDITOR_API CityGmlIFastXmlCallback:IFastXmlCallback
{
public:
	CityGmlIFastXmlCallback(FString modelname);
	virtual ~CityGmlIFastXmlCallback();
	ACityGMLDataType* NewBuilding = nullptr;
	FString modelname;
	AActor* ParentObject;
	PolygonConstructor MeshGenerator;
	FVector relativeLocation;
	bool isRegulationVector = false;
	TArray<EMeshSection> Sections;
	int SectionPolygonNum = 0;
	TEnumAsByte<EMeshSection> MeshSection = NOTSPECIFIED;
	int BuildingCounter = 0;

	UMaterial* NotSpecifiedMaterial;
	UMaterial* GroundMaterial;
	UMaterial* WallMaterial;
	UMaterial* RoofMaterial;
	
	TArray<FString> UnusedData;
	TArray<TCHAR*> DatatoStore;
	TArray<TCHAR*> DatatoDismiss;
	TArray<TCHAR*> AttributestoStore;
	TArray<TCHAR*> AttributestoDismiss;

	
	
	virtual bool ProcessElement(const TCHAR* ElementName, const TCHAR* ElementData, int32 XmlFileLineNumber) override;
	virtual bool ProcessAttribute(const TCHAR* AttributeName, const TCHAR* AttributeValue) override;
	virtual bool ProcessClose(const TCHAR* Element) override;
	virtual bool ProcessXmlDeclaration(const TCHAR* ElementData, int32 XmlFileLineNumber) override;
	virtual bool ProcessComment(const TCHAR* Comment) override;
};
