#include "CityGMLAssetActions.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "CityGMLAsset.h"
#include "Styling/SlateStyle.h"


#define LOCTEXT_NAMESPACE "AssetTypeActions"


FCityGMLAssetActions::FCityGMLAssetActions(EAssetTypeCategories::Type AssetCategory)
{
}


bool FCityGMLAssetActions::CanFilter()
{
	return true;
}


void FCityGMLAssetActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	auto CityGMLAssets = GetTypedWeakObjectPtrs<UCityGMLAsset>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("CityGmlAsset_BuildMesh", "Build Mesh"),
		LOCTEXT("CityGmlAsset_BuildMesh", "Build the mesh from the CityGml File Content."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=, this]
			{
				for (auto& CityGMLAsset : CityGMLAssets)
				{
					if (CityGMLAsset.IsValid() && !CityGMLAsset->Code.IsEmpty())
					{
						Inter.Interpret(CityGMLAsset->Code, CityGMLAsset->GetName());
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("Created CityGML Model!"));
					}
				}
			}),
			FCanExecuteAction::CreateLambda([=, this]
			{
				for (auto& CityGMLAsset : CityGMLAssets)
				{
					if (CityGMLAsset.IsValid() && !CityGMLAsset->Code.IsEmpty())
					{
						return true;
					}
				}
				return false;
			})
		)
	);
}


uint32 FCityGMLAssetActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}


FText FCityGMLAssetActions::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_CityGMLAsset", "CityGML Asset");
}


UClass* FCityGMLAssetActions::GetSupportedClass() const
{
	return UCityGMLAsset::StaticClass();
}


FColor FCityGMLAssetActions::GetTypeColor() const
{
	return FColor::White;
}


bool FCityGMLAssetActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}


#undef LOCTEXT_NAMESPACE
