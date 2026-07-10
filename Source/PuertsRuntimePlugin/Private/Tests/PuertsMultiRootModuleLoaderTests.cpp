#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "PuertsMultiRootModuleLoader.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace
{
class FScopedLoaderTestRoots
{
public:
    FScopedLoaderTestRoots()
    {
        Base = FPaths::ProjectIntermediateDir() / TEXT("PuertsRuntimePluginTests") /
            FGuid::NewGuid().ToString(EGuidFormats::Digits);
        ProjectRoot = Base / TEXT("Project");
        PluginRoot = Base / TEXT("Plugin");
        IFileManager::Get().MakeDirectory(*ProjectRoot, true);
        IFileManager::Get().MakeDirectory(*PluginRoot, true);
    }

    ~FScopedLoaderTestRoots()
    {
        IFileManager::Get().DeleteDirectory(*Base, false, true);
    }

    void Write(const FString& AbsolutePath, const FString& Content = TEXT("module.exports = {};")) const
    {
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(AbsolutePath), true);
        FFileHelper::SaveStringToFile(Content, *AbsolutePath);
    }

    FPuertsMultiRootModuleLoader MakeLoader() const
    {
        return FPuertsMultiRootModuleLoader({
            {TEXT("Project"), ProjectRoot},
            {TEXT("Plugin"), PluginRoot},
        });
    }

    FString Base;
    FString ProjectRoot;
    FString PluginRoot;
};

bool Search(
    FPuertsMultiRootModuleLoader& Loader, const FString& RequiredDir, const FString& Module, FString& Path)
{
    FString DebugPath;
    return Loader.Search(RequiredDir, Module, Path, DebugPath);
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsRootPriorityTest, "PuertsRuntimePlugin.Loader.ProjectOverridesPlugin",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsRootPriorityTest::RunTest(const FString& Parameters)
{
    FScopedLoaderTestRoots Roots;
    Roots.Write(Roots.ProjectRoot / TEXT("node_modules/pkg/index.js"), TEXT("project"));
    Roots.Write(Roots.PluginRoot / TEXT("node_modules/pkg/index.js"), TEXT("plugin"));
    FPuertsMultiRootModuleLoader Loader = Roots.MakeLoader();
    FString Path;

    TestTrue(TEXT("bare package resolves"), Search(Loader, TEXT(""), TEXT("pkg"), Path));
    TestTrue(TEXT("project root wins"), Path.StartsWith(FPuertsMultiRootModuleLoader::CanonicalizePath(Roots.ProjectRoot)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsPluginFallbackTest, "PuertsRuntimePlugin.Loader.PluginFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsPluginFallbackTest::RunTest(const FString& Parameters)
{
    FScopedLoaderTestRoots Roots;
    Roots.Write(Roots.PluginRoot / TEXT("node_modules/pkg/index.js"));
    FPuertsMultiRootModuleLoader Loader = Roots.MakeLoader();
    FString Path;

    TestTrue(TEXT("plugin package resolves"), Search(Loader, TEXT(""), TEXT("pkg"), Path));
    TestTrue(TEXT("plugin root is used"), Path.StartsWith(FPuertsMultiRootModuleLoader::CanonicalizePath(Roots.PluginRoot)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsRelativeImportTest, "PuertsRuntimePlugin.Loader.RelativeImportStaysInPackage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsRelativeImportTest::RunTest(const FString& Parameters)
{
    FScopedLoaderTestRoots Roots;
    const FString PackageDir = Roots.PluginRoot / TEXT("node_modules/pkg/lib");
    Roots.Write(PackageDir / TEXT("helper.js"), TEXT("plugin"));
    Roots.Write(Roots.ProjectRoot / TEXT("helper.js"), TEXT("project"));
    FPuertsMultiRootModuleLoader Loader = Roots.MakeLoader();
    FString Path;

    TestTrue(TEXT("relative module resolves"), Search(Loader, PackageDir, TEXT("./helper"), Path));
    TestTrue(TEXT("relative module remains in package"),
        Path.StartsWith(FPuertsMultiRootModuleLoader::CanonicalizePath(PackageDir)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsMissingRelativeTest, "PuertsRuntimePlugin.Loader.MissingRelativeDoesNotCrossRoots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsMissingRelativeTest::RunTest(const FString& Parameters)
{
    FScopedLoaderTestRoots Roots;
    const FString PackageDir = Roots.PluginRoot / TEXT("node_modules/pkg/lib");
    IFileManager::Get().MakeDirectory(*PackageDir, true);
    Roots.Write(Roots.ProjectRoot / TEXT("helper.js"));
    FPuertsMultiRootModuleLoader Loader = Roots.MakeLoader();
    FString Path;

    TestFalse(TEXT("missing relative module does not cross roots"), Search(Loader, PackageDir, TEXT("./helper"), Path));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsScopedPackageTest, "PuertsRuntimePlugin.Loader.ScopedPackageIsBareModule",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsScopedPackageTest::RunTest(const FString& Parameters)
{
    FScopedLoaderTestRoots Roots;
    Roots.Write(Roots.PluginRoot / TEXT("node_modules/@matrix/puerts-runtime/index.js"));
    FPuertsMultiRootModuleLoader Loader = Roots.MakeLoader();
    FString Path;

    TestTrue(TEXT("scoped package resolves"), Search(Loader, TEXT(""), TEXT("@matrix/puerts-runtime"), Path));
    TestTrue(TEXT("scoped package resolves from plugin"),
        Path.StartsWith(FPuertsMultiRootModuleLoader::CanonicalizePath(Roots.PluginRoot)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsNodeModulesUpwardTest, "PuertsRuntimePlugin.Loader.NodeModulesWalksUpward",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsNodeModulesUpwardTest::RunTest(const FString& Parameters)
{
    FScopedLoaderTestRoots Roots;
    const FString PackageRoot = Roots.PluginRoot / TEXT("node_modules/outer");
    const FString RequiredDir = PackageRoot / TEXT("lib/nested");
    Roots.Write(PackageRoot / TEXT("node_modules/inner/index.js"));
    IFileManager::Get().MakeDirectory(*RequiredDir, true);
    FPuertsMultiRootModuleLoader Loader = Roots.MakeLoader();
    FString Path;

    TestTrue(TEXT("nearest nested dependency resolves"), Search(Loader, RequiredDir, TEXT("inner"), Path));
    TestTrue(TEXT("nearest node_modules wins"), Path.Contains(TEXT("outer/node_modules/inner/index.js")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsExplicitAndPackageTest, "PuertsRuntimePlugin.Loader.ExplicitJsonAndPackageResolution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsExplicitAndPackageTest::RunTest(const FString& Parameters)
{
    FScopedLoaderTestRoots Roots;
    Roots.Write(Roots.ProjectRoot / TEXT("config.json"), TEXT("{}"));
    Roots.Write(Roots.ProjectRoot / TEXT("node_modules/with-main/package.json"), TEXT("{\"main\":\"dist/main.js\"}"));
    Roots.Write(Roots.ProjectRoot / TEXT("node_modules/with-main/dist/main.js"));
    Roots.Write(Roots.ProjectRoot / TEXT("node_modules/with-index/index.js"));
    FPuertsMultiRootModuleLoader Loader = Roots.MakeLoader();
    FString Path;

    TestTrue(TEXT("explicit json resolves"), Search(Loader, TEXT(""), TEXT("config.json"), Path));
    TestTrue(TEXT("package metadata is returned to Puerts"), Search(Loader, TEXT(""), TEXT("with-main"), Path));
    TestTrue(TEXT("package.json selected"), Path.EndsWith(TEXT("with-main/package.json")));
    TestTrue(TEXT("index fallback resolves"), Search(Loader, TEXT(""), TEXT("with-index"), Path));
    TestTrue(TEXT("index.js selected"), Path.EndsWith(TEXT("with-index/index.js")));
    TestFalse(TEXT("json is not inferred"), Search(Loader, TEXT(""), TEXT("config"), Path));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsCanonicalPathTest, "PuertsRuntimePlugin.Loader.CanonicalPathsAreUsable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsCanonicalPathTest::RunTest(const FString& Parameters)
{
    FScopedLoaderTestRoots Roots;
    Roots.Write(Roots.ProjectRoot / TEXT("Main.js"));
    FPuertsMultiRootModuleLoader Loader = Roots.MakeLoader();
    FString Path;
    FString DebugPath;

    TestTrue(TEXT("entry resolves"), Loader.Search(TEXT(""), TEXT("Main"), Path, DebugPath));
    TestFalse(TEXT("read path is absolute"), FPaths::IsRelative(Path));
    TestFalse(TEXT("debug path is absolute"), FPaths::IsRelative(DebugPath));
    TestFalse(TEXT("read path uses forward slashes"), Path.Contains(TEXT("\\")));
    TestFalse(TEXT("debug path uses forward slashes"), DebugPath.Contains(TEXT("\\")));

    TArray<uint8> Content;
    TestTrue(TEXT("read path can load source"), Loader.Load(Path, Content));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsMissingDiagnosticTest, "PuertsRuntimePlugin.Loader.MissingModuleDiagnosticListsRoots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsMissingDiagnosticTest::RunTest(const FString& Parameters)
{
    FScopedLoaderTestRoots Roots;
    FPuertsMultiRootModuleLoader Loader = Roots.MakeLoader();
    FString Path;

    TestFalse(TEXT("missing module is not found"), Search(Loader, Roots.ProjectRoot, TEXT("missing-package"), Path));
    TestTrue(TEXT("diagnostic names request"), Loader.GetLastSearchDiagnostic().Contains(TEXT("missing-package")));
    TestTrue(TEXT("diagnostic names origin"), Loader.GetLastSearchDiagnostic().Contains(Roots.ProjectRoot));
    TestTrue(TEXT("diagnostic lists project root"), Loader.GetLastSearchDiagnostic().Contains(TEXT("Project=")));
    TestTrue(TEXT("diagnostic lists plugin root"), Loader.GetLastSearchDiagnostic().Contains(TEXT("Plugin=")));
    return true;
}

#endif
