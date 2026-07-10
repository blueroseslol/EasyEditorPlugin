using UnrealBuildTool;

public class PuertsEditorPlugin : ModuleRules
{
    public PuertsEditorPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "PuertsRuntimePlugin"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "UnrealEd",
            "LevelEditor",
            "ContentBrowserData",
            "Projects",
            "JsEnv"
        });

        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;
    }
}
