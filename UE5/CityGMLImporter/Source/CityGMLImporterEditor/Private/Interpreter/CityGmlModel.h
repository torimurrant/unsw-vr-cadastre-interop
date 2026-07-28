#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityGmlModel.generated.h"

UCLASS()
class ACityGmlModel : public AActor
{
	GENERATED_BODY()
	
public:	
	ACityGmlModel();
	

protected:
	virtual void BeginPlay() override;

public:	

};
