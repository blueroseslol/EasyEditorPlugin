using UnrealBuildTool;

public class PuertsEditorPlugin : ModuleRules
{
    public PuertsEditorPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;

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
            "DirectoryWatcher",
            "Projects",
            "JsEnv"
        });

        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;
    }
}
