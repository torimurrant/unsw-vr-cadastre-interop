#include "Factories/KMZFactory.h"
#include "Actor/DAEParser.h"
#include "KMZActor.h"

#include "XmlParser.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/FeedbackContext.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "KMZFactory"

UKMZFactory::UKMZFactory()
{
	bCreateNew      = false;
	bEditAfterNew   = false;
	bEditorImport   = true;
	bText           = false;

	Formats.Add(TEXT("kmz;KMZ 3D Strata File"));
	Formats.Add(TEXT("kml;KML Geographic File"));

	SupportedClass = UObject::StaticClass();
}

bool UKMZFactory::FactoryCanImport(const FString& Filename)
{
	FString Ext = FPaths::GetExtension(Filename).ToLower();
	return Ext == TEXT("kmz") || Ext == TEXT("kml");
}

UObject* UKMZFactory::FactoryCreateFile(
	UClass*           InClass,
	UObject*          InParent,
	FName             InName,
	EObjectFlags      Flags,
	const FString&    Filename,
	const TCHAR*      Parms,
	FFeedbackContext* Warn,
	bool&             bOutOperationCanceled)
{
	bOutOperationCanceled = false;

	FString Ext = FPaths::GetExtension(Filename).ToLower();
	FString KMLPath;
	FString ModelsDir;

	if (Ext == TEXT("kmz"))
	{
		if (!ExtractKMZ(Filename, KMLPath, ModelsDir))
		{
			UE_LOG(LogTemp, Error, TEXT("KMZFactory: Failed to extract KMZ: %s"), *Filename);
			return nullptr;
		}
	}
	else
	{
		KMLPath   = Filename;
		ModelsDir = FPaths::GetPath(Filename) / TEXT("models");
	}

	ParseKML(KMLPath, ModelsDir);

	return NewObject<UObject>(InParent, InName, Flags);
}

bool UKMZFactory::ExtractKMZ(const FString& KMZPath,
                               FString&       OutKMLPath,
                               FString&       OutModelsDir)
{
	FString TempDir = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectSavedDir() / TEXT("KMZImport") / FPaths::GetBaseFilename(KMZPath));

	FString TempDirWin = TempDir.Replace(TEXT("/"), TEXT("\\"));
	FString KMZWin     = KMZPath.Replace(TEXT("/"), TEXT("\\"));

	IFileManager::Get().DeleteDirectory(*TempDir, false, true);
	IFileManager::Get().MakeDirectory(*TempDir, true);

	int32 ReturnCode = 0;

#if PLATFORM_WINDOWS
	FString BatchPath = TempDir / TEXT("extract.bat");
	FString BatchContent = FString::Printf(
		TEXT("@echo off\r\npowershell.exe -NoProfile -NonInteractive -Command \"Expand-Archive -Force -LiteralPath '%s' -DestinationPath '%s'\"\r\n"),
		*KMZWin, *TempDirWin);

	FFileHelper::SaveStringToFile(BatchContent, *BatchPath);

	FString StdOut, StdErr;
	FPlatformProcess::ExecProcess(
		*BatchPath,
		TEXT(""),
		&ReturnCode,
		&StdOut,
		&StdErr,
		*TempDir
	);

	UE_LOG(LogTemp, Display, TEXT("KMZFactory: Extraction return code: %d"), ReturnCode);
	if (!StdOut.IsEmpty()) UE_LOG(LogTemp, Display, TEXT("KMZFactory: stdout: %s"), *StdOut);
	if (!StdErr.IsEmpty()) UE_LOG(LogTemp, Warning, TEXT("KMZFactory: stderr: %s"), *StdErr);

#else
	FString Args = FString::Printf(TEXT("-o \"%s\" -d \"%s\""), *KMZPath, *TempDir);
	FString StdOut, StdErr;
	FPlatformProcess::ExecProcess(TEXT("/usr/bin/unzip"), *Args,
		&ReturnCode, &StdOut, &StdErr);
#endif

	OutKMLPath   = TempDir / TEXT("doc.kml");
	OutModelsDir = TempDir / TEXT("models");

	UE_LOG(LogTemp, Display, TEXT("KMZFactory: Temp dir: %s"), *TempDir);
	UE_LOG(LogTemp, Display, TEXT("KMZFactory: Expecting KML at: %s"), *OutKMLPath);

	TArray<FString> FoundFiles;
	IFileManager::Get().FindFilesRecursive(FoundFiles, *TempDir, TEXT("*"), true, false);
	for (const FString& F : FoundFiles)
	{
		UE_LOG(LogTemp, Display, TEXT("KMZFactory: Found file: %s"), *F);
	}

	if (!FPaths::FileExists(OutKMLPath))
	{
		UE_LOG(LogTemp, Error,
			TEXT("KMZFactory: doc.kml not found after extraction. See files listed above."));
		return false;
	}

	return true;
}

void UKMZFactory::ParseKML(const FString& KMLPath, const FString& ModelsDir)
{
	FXmlFile XmlFile;
	if (!XmlFile.LoadFile(KMLPath))
	{
		UE_LOG(LogTemp, Error, TEXT("KMZFactory: Cannot load KML: %s"), *KMLPath);
		return;
	}

	const FXmlNode* Root = XmlFile.GetRootNode();
	if (!Root) return;

	const FXmlNode* Document = Root->FindChildNode(TEXT("Document"));
	if (!Document) return;

	const FXmlNode* Folder = Document->FindChildNode(TEXT("Folder"));
	const FXmlNode* SearchRoot = Folder ? Folder : Document;

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("KMZFactory: No editor world available"));
		return;
	}

	int32 SpawnedCount = 0;

	for (const FXmlNode* Placemark : SearchRoot->GetChildrenNodes())
	{
		if (Placemark->GetTag() != TEXT("Placemark")) continue;

		FString PlacemarkId = Placemark->GetAttribute(TEXT("id"));
		const FXmlNode* NameNode = Placemark->FindChildNode(TEXT("name"));
		FString ActorLabel = NameNode ? NameNode->GetContent() : PlacemarkId;

		FString UniqueId, PlanNumber, Type, Label;
		int32   LotNumber   = 0;
		int32   Entitlement = 0;

		const FXmlNode* ExtData = Placemark->FindChildNode(TEXT("ExtendedData"));
		if (ExtData)
		{
			const FXmlNode* SchemaData = ExtData->FindChildNode(TEXT("SchemaData"));
			if (SchemaData)
			{
				for (const FXmlNode* SD : SchemaData->GetChildrenNodes())
				{
					if (SD->GetTag() != TEXT("SimpleData")) continue;
					FString AttrName  = SD->GetAttribute(TEXT("name"));
					FString AttrValue = SD->GetContent().TrimStartAndEnd();

					if      (AttrName == TEXT("uniqueId"))    UniqueId    = AttrValue;
					else if (AttrName == TEXT("planNumber"))  PlanNumber  = AttrValue;
					else if (AttrName == TEXT("lotNumber"))   LotNumber   = FCString::Atoi(*AttrValue);
					else if (AttrName == TEXT("type"))        Type        = AttrValue;
					else if (AttrName == TEXT("label"))       Label       = AttrValue;
					else if (AttrName == TEXT("entitlement")) Entitlement = FCString::Atoi(*AttrValue);
				}
			}
		}

		FString DAERelPath;
		const FXmlNode* ModelNode = Placemark->FindChildNode(TEXT("Model"));
		if (ModelNode)
		{
			const FXmlNode* LinkNode = ModelNode->FindChildNode(TEXT("Link"));
			if (LinkNode)
			{
				const FXmlNode* HrefNode = LinkNode->FindChildNode(TEXT("href"));
				if (HrefNode)
				{
					DAERelPath = HrefNode->GetContent().TrimStartAndEnd();
				}
			}
		}

		if (DAERelPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("KMZFactory: Placemark '%s' has no Model/Link/href — skipping"),
				*ActorLabel);
			continue;
		}

		FString KMLDir  = FPaths::GetPath(KMLPath);
		FString DAEPath = FPaths::ConvertRelativePathToFull(KMLDir / DAERelPath);

		if (!FPaths::FileExists(DAEPath))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("KMZFactory: DAE file not found: %s"), *DAEPath);
			continue;
		}

		FDAEMesh Mesh;
		if (!FDAEParser::Parse(DAEPath, Mesh))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("KMZFactory: Failed to parse DAE: %s"), *DAEPath);
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = FName(*ActorLabel);

		AKMZActor* Actor = World->SpawnActor<AKMZActor>(
			AKMZActor::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams);

		if (!Actor) continue;

		Actor->SetActorLabel(ActorLabel);

		Actor->UniqueId    = UniqueId;
		Actor->PlanNumber  = PlanNumber;
		Actor->LotNumber   = LotNumber;
		Actor->Type        = Type;
		Actor->Label       = Label;
		Actor->Entitlement = Entitlement;

		Actor->BuildMesh(Mesh.Vertices, Mesh.Triangles, Mesh.Normals);

		SpawnedCount++;
	}

	UE_LOG(LogTemp, Display,
		TEXT("KMZFactory: Spawned %d KMZ actors from %s"), SpawnedCount, *KMLPath);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
			FString::Printf(TEXT("Import complete: %d lots imported"), SpawnedCount));
	}
}

#undef LOCTEXT_NAMESPACE
