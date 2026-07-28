#include "Factories/CityGMLAssetFactory.h"

#include "AssetTypeCategories.h"
#include "CityGMLImporter/Public/CityGMLAsset.h"

#define LOCTEXT_NAMESPACE "CityGmlAsset"

UCityGMLAssetFactory::UCityGMLAssetFactory(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UCityGMLAsset::StaticClass();
}

UObject* UCityGMLAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	auto newAsset = NewObject<UCityGMLAsset>(InParent, InClass, InName, Flags);
	return newAsset;
}

bool UCityGMLAssetFactory::ShouldShowInNewMenu() const
{
	return true;
}

FText UCityGMLAssetFactory::GetDisplayName() const
{
	return LOCTEXT("CityGMLAssetDescriptions", "CityGmlAsset");
}

uint32 UCityGMLAssetFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}



