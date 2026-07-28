#pragma once

#include "AssetTypeActions_Base.h"
#include "CityGMLAsset.h"
#include "Templates/SharedPointer.h"

#include "Interpreter/PolygonConstructor.h"
#include "Interpreter/GMLInterpreter.h"

class FCityGMLAssetActions
	: public FAssetTypeActions_Base
{
public:
	
	FCityGMLAssetActions(const EAssetTypeCategories::Type AssetCategory);

public:

	virtual bool CanFilter() override;
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual uint32 GetCategories() override;
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;

	int PolygonIndex;
	GMLInterpreter Inter;


private:
	

};