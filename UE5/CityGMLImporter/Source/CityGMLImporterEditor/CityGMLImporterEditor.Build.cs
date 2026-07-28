using UnrealBuildTool;

public class CityGMLImporterEditor : ModuleRules
{
	public CityGMLImporterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				"../Plugins/Runtime/ProceduralMeshComponent",
				"Runtime/XmlParser/Public/FastXml.h",
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"ProceduralMeshComponent",
				"XmlParser",
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"ProceduralMeshComponent",
				"XmlParser",
				"CityGMLImporter",
				"UnrealEd",
				"MeshDescription",
				"StaticMeshDescription"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ContentBrowser",
				"Core",
				"CoreUObject",
				"DesktopWidgets",
				"EditorStyle",
				"Engine",
				"InputCore",
				"Projects",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"ToolMenus", 
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				"AssetTools",
				"MainFrame",
			}
			);
	}
}
