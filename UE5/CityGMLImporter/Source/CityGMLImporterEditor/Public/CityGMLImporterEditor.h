#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Interpreter/PolygonConstructor.h"
#include "Interpreter/GMLInterpreter.h"
#include "Interpreter/CityGmlIFastXmlCallback.h"
#include "ProceduralMeshComponent.h"

class FCityGMLImporterEditorModule : public IModuleInterface
{
public:
	
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> ActorToSpawn;

	int PolygonIndex;
	GMLInterpreter Inter;
	
private:

	void RegisterMenus();


private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
