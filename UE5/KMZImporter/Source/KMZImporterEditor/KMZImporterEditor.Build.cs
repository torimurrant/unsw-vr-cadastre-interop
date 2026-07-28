using UnrealBuildTool;

public class KMZImporterEditor : ModuleRules
{
	public KMZImporterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"ProceduralMeshComponent",
			"XmlParser",
			"KMZImporter",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetTools",
			"ContentBrowser",
			"EditorStyle",
			"InputCore",
			"Projects",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd",
		});

		DynamicallyLoadedModuleNames.AddRange(new string[]
		{
			"MainFrame",
		});
	}
}
