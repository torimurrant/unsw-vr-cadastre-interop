#include "KMZImporterEditor.h"
#include "Modules/ModuleManager.h"
#include "Factories/KMZFactory.h"

#define LOCTEXT_NAMESPACE "FKMZImporterEditorModule"

void FKMZImporterEditorModule::StartupModule()
{
}

void FKMZImporterEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKMZImporterEditorModule, KMZImporterEditor)
