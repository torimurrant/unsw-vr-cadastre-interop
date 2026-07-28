#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "KMZFactory.generated.h"

UCLASS()
class UKMZFactory : public UFactory
{
	GENERATED_BODY()

public:
	UKMZFactory();

	virtual UObject* FactoryCreateFile(
		UClass*            InClass,
		UObject*           InParent,
		FName              InName,
		EObjectFlags       Flags,
		const FString&     Filename,
		const TCHAR*       Parms,
		FFeedbackContext*  Warn,
		bool&              bOutOperationCanceled) override;

	virtual bool FactoryCanImport(const FString& Filename) override;

private:
	bool ExtractKMZ(const FString& KMZPath, FString& OutKMLPath, FString& OutModelsDir);

	void ParseKML(const FString& KMLPath, const FString& ModelsDir);
};
