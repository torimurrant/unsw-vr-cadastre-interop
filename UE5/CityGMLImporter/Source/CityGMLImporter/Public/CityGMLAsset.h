#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

#include "CityGMLAsset.generated.h"

UCLASS(BlueprintType,HideCategories =(Object))
class CITYGMLIMPORTER_API UCityGMLAsset : public UObject
{

	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "CityGMLAsset")
	FString Code;
};
