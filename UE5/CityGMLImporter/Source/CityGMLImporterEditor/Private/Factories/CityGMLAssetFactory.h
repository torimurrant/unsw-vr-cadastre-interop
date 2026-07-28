#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "UObject/ObjectMacros.h"

#include "CityGMLAssetFactory.generated.h"

UCLASS()
class UCityGMLAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
	
public:
	
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override;
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
};

