using UnrealBuildTool;

public class PuertsRuntimePlugin : ModuleRules
{
    public PuertsRuntimePlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "JsEnv"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Projects"
        });

        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;

        RuntimeDependencies.Add("$(PluginDir)/Content/JavaScript/...*.js", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(PluginDir)/Content/JavaScript/...*.mjs", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(PluginDir)/Content/JavaScript/...*.cjs", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(PluginDir)/Content/JavaScript/...*.json", StagedFileType.NonUFS);

        RuntimeDependencies.Add("$(ProjectDir)/Content/JavaScript/Main.js", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/JavaScript/puerts/...*.js", StagedFileType.NonUFS);
    }
}
