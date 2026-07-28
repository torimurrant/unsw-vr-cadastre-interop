// Copyright Epic Games, Inc. All Rights Reserved.

#include "CityGMLImporterEditor.h"

#include "HairStrandsInterface.h"
#include "Containers/Array.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "ToolMenus.h"

#include "AssetTools/CityGMLAssetActions.h"

static const FName CityGMLTabName("CityGML");

#define LOCTEXT_NAMESPACE "FCityGMLImporterEditorModule"

void FCityGMLImporterEditorModule::StartupModule()
{
	//Register AssetActions for CityGML Asset
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	EAssetTypeCategories::Type AssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("CityGml")), LOCTEXT("CityGmlAssetcategory","CityGmlAsset"));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FCityGMLAssetActions(AssetCategoryBit)));
	
}

void FCityGMLImporterEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);
}

/*void FCityGMLImporterEditorModule::PluginButtonClicked()
{
	Inter.Interpret();
	UClass* a = LoadClass<APolygonConstructor>(nullptr, TEXT("/Script/CityGML.Polygonconstructor"));
	if (a != NULL) {
		ActorToSpawn = a;
		UE_LOG(LogTemp, Warning, TEXT("OK"));
	}
	FVector const ObjLocation = FVector::ZeroVector;
	APolygonConstructor* tempObject =(APolygonConstructor*) GEditor->GetEditorWorldContext().World()->SpawnActor(APolygonConstructor::StaticClass(), &ObjLocation);
	FProcMeshTangent Tangent = tempObject->ResetMesh();
	int32 TriIndex = 0;
	for (int i = 0; i < Inter.cllback->ArrayOfPolygons.Num(); i++)
	{
		tempObject->Polygon = Inter.getPolygon(i);
		tempObject->AddPolygon(tempObject->Polygon, TriIndex, Tangent);
	}
	TriIndex = 0;
	tempObject->GenerateMesh();
	tempObject->ResetMesh();
}*/
	

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCityGMLImporterEditorModule, CityGMLImporterEditor)