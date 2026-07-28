#include "Interpreter/CityGmlIFastXmlCallback.h"

#include <tiff.h>

#include "Interpreter/CityGmlModel.h"

#include "ProceduralMeshConversion.h"

CityGmlIFastXmlCallback::CityGmlIFastXmlCallback(FString modelname)
{
	NotSpecifiedMaterial	= LoadObject<UMaterial>(nullptr,TEXT("Material'/CityGMLImporter/NotSpecifiedMaterial.NotSpecifiedMaterial'"));
	GroundMaterial			= LoadObject<UMaterial>(nullptr,TEXT("Material'/CityGMLImporter/GroundMaterial.GroundMaterial'"));
	WallMaterial			= LoadObject<UMaterial>(nullptr,TEXT("Material'/CityGMLImporter/WallMaterial.WallMaterial'"));
	RoofMaterial			= LoadObject<UMaterial>(nullptr,TEXT("Material'/CityGMLImporter/RoofMaterial.RoofMaterial'"));
	this->modelname = modelname;
	ParentObject = GEditor->GetEditorWorldContext().World()->SpawnActor(ACityGmlModel::StaticClass(), &FVector::ZeroVector);
	ParentObject->SetActorLabel(modelname);
}

CityGmlIFastXmlCallback::~CityGmlIFastXmlCallback()
{
}

bool CityGmlIFastXmlCallback::ProcessElement(const TCHAR* ElementName, const TCHAR* ElementData, int32 XmlFileLineNumber)
{
	TCHAR* GroundSurface =(TCHAR*) L"bldg:GroundSurface";
	if (_tcscmp(ElementName,GroundSurface)==0) {
		MeshSection = GROUND;
		return true;
	}

	TCHAR* WallSurface =(TCHAR*) L"bldg:WallSurface";
	if (_tcscmp(ElementName,WallSurface)==0) {
		MeshSection = WALL;
		return true;
	}

	TCHAR* RoofSurface =(TCHAR*) L"bldg:RoofSurface";
	if (_tcscmp(ElementName,RoofSurface)==0) {
		MeshSection = ROOF;
		return true;
	}
	
	TCHAR* BuildingPart = (TCHAR*) L"bldg:consistsOfBuildingPart";
	if (_tcscmp(ElementName,BuildingPart)==0)
	{
		return true;
	}
	TCHAR* Name = (TCHAR*) L"gml:name";
	if (_tcscmp(ElementName,Name)==0)
	{
		return true;
	}
	TCHAR* Function = (TCHAR*) L"bldg:function";
	if (_tcscmp(ElementName,Function)==0)
	{
		NewBuilding->Function=(ElementData);
		return true;
	}
	TCHAR* Usage = (TCHAR*) L"bldg:usage";
	if (_tcscmp(ElementName,Usage)==0)
	{
		NewBuilding->Usage=(ElementData);
		return true;
	}
	TCHAR* YearOfConstruction = (TCHAR*) L"bldg:yearOfConstruction";
	if (_tcscmp(ElementName,YearOfConstruction)==0)
	{
		NewBuilding->YearOfConstruction=(ElementData);
		return true;
	}
	TCHAR* YearOfDemolition = (TCHAR*) L"bldg:yearOfDemolition";
	if (_tcscmp(ElementName,YearOfDemolition)==0)
	{
		NewBuilding->YearOfDemolition=(ElementData);
		return true;
	}
	TCHAR* RoofType = (TCHAR*) L"bldg:roofType";
	if (_tcscmp(ElementName,RoofType)==0)
	{
		NewBuilding->RoofType=(ElementData);
		return true;
	}
	TCHAR* measuredHeight = (TCHAR*) L"bldg:measuredHeight";
	if (_tcscmp(ElementName,measuredHeight)==0)
	{
		NewBuilding->MeasuredHeight=(ElementData);
		return true;
	}
	TCHAR* StoreysAboveGround = (TCHAR*) L"bldg:storeysAboveGround";
	if (_tcscmp(ElementName,StoreysAboveGround)==0)
	{
		NewBuilding->StoreysAboveGround=(ElementData);
		return true;
	}
	TCHAR* StoreysBelowGround = (TCHAR*) L"bldg:storeysBelowGround";
	if (_tcscmp(ElementName,StoreysBelowGround)==0)
	{
		NewBuilding->StoreysBelowGround=(ElementData);
		return true;
	}
	TCHAR* StoreysHeightsAboveGround = (TCHAR*) L"bldg:storeysHeightsAboveGround";
	if (_tcscmp(ElementName,StoreysHeightsAboveGround)==0)
	{
		NewBuilding->StoreysHeightAboveGround=(ElementData);
		return true;
	}
	TCHAR* StoreysHeightsBelowGround = (TCHAR*) L"bldg:storeysHeightsBelowGround";
	if (_tcscmp(ElementName,StoreysHeightsBelowGround)==0)
	{
		NewBuilding->StoreysHeightBelowGround=(ElementData);
		return true;
	}
	TCHAR* Address = (TCHAR*) L"bldg:address"; //Not implemented yet
	if (_tcscmp(ElementName,Address)==0)
	{
		NewBuilding->gotAddress = true;
		return true;
	}
	if (NewBuilding && NewBuilding->gotAddress)
	{
		TCHAR* CountryName = (TCHAR*) L"xAL:CountryName"; 
		if (_tcscmp(ElementName,CountryName)==0)
		{
			NewBuilding->CountryName=(ElementData);
			return true;
		}
		CountryName = (TCHAR*) L"xal:CountryName"; 
		if (_tcscmp(ElementName,CountryName)==0)
		{
			NewBuilding->CountryName=(ElementData);
			return true;
		}
		TCHAR* LocalityName = (TCHAR*) L"xAL:LocalityName"; 
		if (_tcscmp(ElementName,LocalityName)==0)
		{
			NewBuilding->LocalityName=(ElementData);
			return true;
		}
		LocalityName = (TCHAR*) L"xal:LocalityName"; 
		if (_tcscmp(ElementName,LocalityName)==0)
		{
			NewBuilding->LocalityName=(ElementData);
			return true;
		}
		TCHAR* ThoroughfareName = (TCHAR*) L"xAL:ThoroughfareName";
		if (_tcscmp(ElementName,ThoroughfareName)==0)
		{
			NewBuilding->ThoroughfareName=(ElementData);
			return true;
		}
		ThoroughfareName = (TCHAR*) L"xal:ThoroughfareName";
		if (_tcscmp(ElementName,ThoroughfareName)==0)
		{
			NewBuilding->ThoroughfareName=(ElementData);
			return true;
		}
		TCHAR* ThoroughfareNumber = (TCHAR*) L"xAL:ThoroughfareNumber"; 
		if (_tcscmp(ElementName,ThoroughfareNumber)==0)
		{
			NewBuilding->ThoroughfareNumber=(ElementData);
			return true;
		}
		ThoroughfareNumber = (TCHAR*) L"xal:ThoroughfareNumber"; 
		if (_tcscmp(ElementName,ThoroughfareNumber)==0)
		{
			NewBuilding->ThoroughfareNumber=(ElementData);
			return true;
		}
		TCHAR* DepartmentName = (TCHAR*) L"xAL:DepartmentName"; 
		if (_tcscmp(ElementName,DepartmentName)==0)
		{
			NewBuilding->DepartmentName=(ElementData);
			return true;
		}
		DepartmentName = (TCHAR*) L"xal:DepartmentName"; 
		if (_tcscmp(ElementName,DepartmentName)==0)
		{
			NewBuilding->DepartmentName=(ElementData);
			return true;
		}
		TCHAR* DependentLocalityName = (TCHAR*) L"xAL:DependentLocalityName"; 
		if (_tcscmp(ElementName,DependentLocalityName)==0)
		{
			NewBuilding->DependentLocalityName=(ElementData);
			return true;
		}
		DependentLocalityName = (TCHAR*) L"xal:DependentLocalityName"; 
		if (_tcscmp(ElementName,DependentLocalityName)==0)
		{
			NewBuilding->DependentLocalityName=(ElementData);
			return true;
		}
		TCHAR* DependentLocalityNumber = (TCHAR*) L"xAL:DependentLocalityNumber"; 
		if (_tcscmp(ElementName,DependentLocalityNumber)==0)
		{
			NewBuilding->DependentLocalityNumber=(ElementData);
			return true;
		}
		DependentLocalityNumber = (TCHAR*) L"xal:DependentLocalityNumber"; 
		if (_tcscmp(ElementName,DependentLocalityNumber)==0)
		{
			NewBuilding->DependentLocalityNumber=(ElementData);
			return true;
		}
		TCHAR* PostalCodeNumber = (TCHAR*) L"xAL:PostalCodeNumber"; 
		if (_tcscmp(ElementName,PostalCodeNumber)==0)
		{
			NewBuilding->PostalCodeNumber=(ElementData);
			return true;
		}
		PostalCodeNumber = (TCHAR*) L"xal:PostalCodeNumber"; 
		if (_tcscmp(ElementName,PostalCodeNumber)==0)
		{
			NewBuilding->PostalCodeNumber=(ElementData);
			return true;
		}
		TCHAR* PostalCodeNumberExtension = (TCHAR*) L"xAL:PostalCodeNumberExtension"; 
		if (_tcscmp(ElementName,PostalCodeNumberExtension)==0)
		{
			NewBuilding->PostalCodeNumberExtension=(ElementData);
			return true;
		}
		PostalCodeNumberExtension = (TCHAR*) L"xal:PostalCodeNumberExtension"; 
		if (_tcscmp(ElementName,PostalCodeNumberExtension)==0)
		{
			NewBuilding->PostalCodeNumberExtension=(ElementData);
			return true;
		}
	}
	TCHAR* MultiSLOD1 = (TCHAR*) L"bldg:lod1MultiSurface";
	if (_tcscmp(ElementName,MultiSLOD1)==0)
	{
		NewBuilding->LOD="LOD1";
		return true;
	}
	TCHAR* MultiSLOD2 = (TCHAR*) L"bldg:lod2MultiSurface"; 
	if (_tcscmp(ElementName,MultiSLOD2)==0)
	{
		NewBuilding->LOD="LOD2";
		return true;
	}
	TCHAR* MultiSLOD3 = (TCHAR*) L"bldg:lod3MultiSurface"; 
	if (_tcscmp(ElementName,MultiSLOD3)==0)
	{
		NewBuilding->LOD="LOD3";
		return true;
	}
	TCHAR* MultiSLOD4 = (TCHAR*) L"bldg:lod4MultiSurface"; 
	if (_tcscmp(ElementName,MultiSLOD4)==0)
	{
		NewBuilding->LOD="LOD4";
		return true;
	}
	TCHAR* MultiCLOD2 = (TCHAR*) L"bldg:lod2MultiCurve"; 
	if (_tcscmp(ElementName,MultiCLOD2)==0)
	{
		NewBuilding->LOD="LOD2";
		return true;
	}
	TCHAR* MultiCLOD3 = (TCHAR*) L"bldg:lod3MultiCurve"; 
	if (_tcscmp(ElementName,MultiCLOD3)==0)
	{
		NewBuilding->LOD="LOD3";
		return true;
	}
	TCHAR* MultiCLOD4 = (TCHAR*) L"bldg:lod4MultiCurve"; 
	if (_tcscmp(ElementName,MultiCLOD4)==0)
	{
		NewBuilding->LOD="LOD4";
		return true;
	}
	TCHAR* FootPrintLOD0 = (TCHAR*) L"bldg:lod0FootPrint"; 
	if (_tcscmp(ElementName,FootPrintLOD0)==0)
	{
		NewBuilding->LOD="LOD0";
		return true;
	}
	TCHAR* RoofEdgeLOD0 = (TCHAR*) L"bldg:lod0RoodEdge"; 
	if (_tcscmp(ElementName,RoofEdgeLOD0)==0)
	{
		NewBuilding->LOD="LOD0";
		return true;
	}
	TCHAR* SolidLOD1 = (TCHAR*) L"bldg:lod1Solid"; 
	if (_tcscmp(ElementName,SolidLOD1)==0)
	{
		NewBuilding->LOD="LOD1";
		return true;
	}
	TCHAR* SolidLOD2 = (TCHAR*) L"bldg:lod2Solid"; 
	if (_tcscmp(ElementName,SolidLOD2)==0)
	{
		NewBuilding->LOD="LOD2";
		return true;
	}
	TCHAR* SolidLOD3 = (TCHAR*) L"bldg:lod3Solid"; 
	if (_tcscmp(ElementName,SolidLOD3)==0)
	{
		NewBuilding->LOD="LOD3";
		return true;
	}
	TCHAR* SolidLOD4 = (TCHAR*) L"bldg:lod4Solid"; 
	if (_tcscmp(ElementName,SolidLOD4)==0)
	{
		NewBuilding->LOD="LOD4";
		return true;
	}
	
	TCHAR* posList =(TCHAR*) L"gml:posList";
	if (_tcscmp(ElementName,posList)==0) {
		Sections.Add(MeshSection);
		NewBuilding->Polygons.Add(ACityGMLDataType::ProcessVerticesToPolygon(ElementData));
		return true;
	}

	FString cityObjMember =(TCHAR*) L"cityObjectMember";
	FString convertedElement = ElementName;
	if (convertedElement.Contains(cityObjMember))
	{

		NewBuilding = static_cast<ACityGMLDataType*>(GEditor->GetEditorWorldContext().World()->SpawnActor(
			ACityGMLDataType::StaticClass(), &FVector::ZeroVector));
		return true;
	}
	if (ElementData!=nullptr)
	{
		TCHAR* genvalue = (TCHAR*) L"gen:value";
		if(_tcscmp(ElementName,genvalue)==0)UnusedData.Last() = UnusedData.Last()+" : "+(FString) ElementData;
		else UnusedData.Last() = UnusedData.Last()+" : "+(FString)ElementName+" : "+(FString) ElementData;
		UnusedData.Add("");
		return true;
	}
	if(ElementData == nullptr)
	{
		if (UnusedData.IsEmpty())
		{
			UnusedData.Add("");
		}
		if(_tcscmp(ElementName,L"gen:stringAttribute")==0) return  true;
		UnusedData.Last() = ""+(FString)ElementName;
	}
	return true;
}
bool CityGmlIFastXmlCallback::ProcessAttribute(const TCHAR* AttributeName, const TCHAR* AttributeValue)
{
	if (UnusedData.IsEmpty())
	{
		UnusedData.Add("");
	}
	if(_tcscmp(AttributeName,L"gml:id")==0)return true;
	if(_tcscmp(AttributeName,L"srsDimension")==0) return true;
	if(_tcscmp(AttributeName,L"srsName")==0) return true;
	if(_tcscmp(AttributeName,L"Type")==0) return true;
	if(_tcscmp(AttributeName,L"xlink:href")==0) return true;
	if(_tcscmp(AttributeName,L"ring")==0) return true;
	if(_tcscmp(AttributeName,L"uri")==0) return true;
	if(_tcscmp(AttributeName,L"uom")==0) return true;
	if (_tcscmp(AttributeName,L"name")==0)
	{
		UnusedData.Last() = ""+(FString)AttributeValue;
	}
	else
	{
		UE_LOG(LogTemp,Display,TEXT("%s : %s"),AttributeName,AttributeValue)
	}
	return true;
}

bool CityGmlIFastXmlCallback::ProcessClose(const TCHAR* Element)
{
	FString cityObjMember =(TCHAR*) L"cityObjectMember";
	FString convertedElement = Element;
	if (convertedElement.Contains(cityObjMember))
	{
		int32 TriIndex = 0;
		FProcMeshTangent Tangent = MeshGenerator.ResetMesh();
		if (!isRegulationVector)
		{
			MeshGenerator.RegulationVector = NewBuilding->Polygons[0].ArrayOfVertices[0];
			isRegulationVector = true;
		}
		for (int currentMeshSection = 0; currentMeshSection < 4; currentMeshSection++)
		{
			bool isEmpty = true;
			for (int i = 0; i < NewBuilding->Polygons.Num(); i++)
			{
				if (Sections[i] == currentMeshSection)
				{
					isEmpty = false;
					TArray<FVector> Polygon = NewBuilding->Polygons[i].ArrayOfVertices;
					MeshGenerator.isfirsttime = true;
					MeshGenerator.AddPolygon(Polygon, TriIndex);
				}
			}
			if (!isEmpty)
			{
				relativeLocation = MeshGenerator.GenerateMesh(currentMeshSection,NewBuilding->ThisMeshProc,NewBuilding->ThisMesh);
				relativeLocation.operator*=(-1);
				TriIndex = 0;
				MeshGenerator.ResetMesh();
			}
		}
		NewBuilding->ThisMeshProc->SetMaterial(NOTSPECIFIED, NotSpecifiedMaterial);
		NewBuilding->ThisMeshProc->SetMaterial(GROUND, GroundMaterial);
		NewBuilding->ThisMeshProc->SetMaterial(WALL, WallMaterial);
		NewBuilding->ThisMeshProc->SetMaterial(ROOF, RoofMaterial);

		NewBuilding->SetActorLabel("Building"+ FString::FromInt(BuildingCounter));
		NewBuilding->ThisMesh->SetStaticMesh(MeshGenerator.ConvertToStaticMesh(NewBuilding->ThisMeshProc, modelname, BuildingCounter++ ));
		

		
		
		Sections.Empty();
		TriIndex = 0;
				
		NewBuilding->UnusedData = UnusedData;
		UnusedData.Reset();

		NewBuilding->SetActorLocation(relativeLocation);
		NewBuilding->AttachToActor(ParentObject, FAttachmentTransformRules::KeepRelativeTransform);
		NewBuilding->ThisMeshProc->DestroyComponent();
		NewBuilding->ThisMeshProc = nullptr;
		return true;
	}
		TCHAR* CityModel =(TCHAR*) L"CityModel";
	if (_tcscmp(Element,CityModel)==0)
	{
		UE_LOG(LogTemp,Warning,TEXT("Breakcount: %f; Polygons: %f"),MeshGenerator.breaks,MeshGenerator.Polygons);
		MeshGenerator.breaks = 0;
		MeshGenerator.Polygons = 0;
		isRegulationVector = false;
		return true;
	}
	CityModel =(TCHAR*) L"core:CityModel";
	if (_tcscmp(Element,CityModel)==0)
	{
		UE_LOG(LogTemp,Warning,TEXT("Breakcount: %f; Polygons: %f"),MeshGenerator.breaks,MeshGenerator.Polygons);
		MeshGenerator.breaks = 0;
		MeshGenerator.Polygons = 0;
		isRegulationVector = false;
		return true;
	}

	TCHAR* GroundSurface =(TCHAR*) L"bldg:GroundSurface";
	if (_tcscmp(Element,GroundSurface)==0) {
		MeshSection = NOTSPECIFIED;
		return true;
	}

	TCHAR* WallSurface =(TCHAR*) L"bldg:WallSurface";
	if (_tcscmp(Element,WallSurface)==0) {
		MeshSection = NOTSPECIFIED;
		return true;
	}

	TCHAR* RoofSurface =(TCHAR*) L"bldg:RoofSurface";
	if (_tcscmp(Element,RoofSurface)==0) {
		MeshSection = NOTSPECIFIED;
		return true;
	}
	
	return true;
}

bool CityGmlIFastXmlCallback::ProcessXmlDeclaration(const TCHAR* ElementData, int32 XmlFileLineNumber)
{
	return true;
}

bool CityGmlIFastXmlCallback::ProcessComment(const TCHAR* Comment)
{
	return true;
}
