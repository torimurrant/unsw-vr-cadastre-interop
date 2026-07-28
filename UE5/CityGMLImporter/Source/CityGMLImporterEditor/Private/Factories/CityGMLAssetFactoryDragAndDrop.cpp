#include "Factories/CityGMLAssetFactoryDragAndDrop.h"

#include "CityGMLAsset.h"
#include "Misc/FileHelper.h"

UCityGMLAssetFactoryDragAndDrop::UCityGMLAssetFactoryDragAndDrop(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Formats.Add(FString(TEXT("gml;")) + NSLOCTEXT("UCityGmlAssetFactoryDragAndDrop", "FormatGml", "Gml File").ToString());
	SupportedClass = UCityGMLAsset::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UCityGMLAssetFactoryDragAndDrop::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UCityGMLAsset* CityGmlAsset = nullptr;
	FString TextString;

	if (FFileHelper::LoadFileToString(TextString, *Filename))
	{
		CityGmlAsset = NewObject<UCityGMLAsset>(InParent, InClass, InName, Flags);
		CityGmlAsset->Code = TextString;
	}

	bOutOperationCanceled = false;

	return CityGmlAsset;
}
