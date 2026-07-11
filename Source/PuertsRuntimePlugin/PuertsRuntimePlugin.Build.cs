using System;
using System.IO;
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

        if (Target.ProjectFile != null)
        {
            string ProjectJavaScriptDir = Path.Combine(
                Target.ProjectFile.Directory.FullName, "Content", "JavaScript");
            if (Directory.Exists(ProjectJavaScriptDir))
            {
                foreach (string SourceFile in Directory.GetFiles(
                    ProjectJavaScriptDir, "*", SearchOption.TopDirectoryOnly))
                {
                    string Extension = Path.GetExtension(SourceFile);
                    if (Extension.Equals(".js", StringComparison.OrdinalIgnoreCase)
                        || Extension.Equals(".mjs", StringComparison.OrdinalIgnoreCase)
                        || Extension.Equals(".cjs", StringComparison.OrdinalIgnoreCase)
                        || Extension.Equals(".json", StringComparison.OrdinalIgnoreCase))
                    {
                        RuntimeDependencies.Add(
                            "$(ProjectDir)/Content/JavaScript/" + Path.GetFileName(SourceFile),
                            StagedFileType.NonUFS);
                    }
                }
            }
        }
        RuntimeDependencies.Add("$(ProjectDir)/Content/JavaScript/puerts/...*.js", StagedFileType.NonUFS);
    }
}
