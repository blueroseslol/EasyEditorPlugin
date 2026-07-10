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
    }
}
