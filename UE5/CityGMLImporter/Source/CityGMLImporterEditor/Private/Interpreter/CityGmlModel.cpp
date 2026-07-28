#include "Interpreter/CityGmlModel.h"

ACityGmlModel::ACityGmlModel()
{
	PrimaryActorTick.bCanEverTick = true;
	
	RootComponent = this->CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	

}

void ACityGmlModel::BeginPlay()
{
	Super::BeginPlay();
	
}


