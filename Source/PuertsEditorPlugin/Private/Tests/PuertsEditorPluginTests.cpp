#include "Misc/AutomationTest.h"
#include "PuertsEditorEnvironment.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsEditorScriptRootsTest,
    "PuertsEditorPlugin.Environment.ProjectRootPrecedesPluginRoot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsEditorScriptRootsTest::RunTest(const FString& Parameters)
{
    const TArray<FPuertsScriptRoot> Roots = BuildPuertsEditorScriptRoots();

    TestEqual(TEXT("two editor roots"), Roots.Num(), 2);
    if (Roots.Num() == 2)
    {
        TestEqual(TEXT("project root is first"), Roots[0].Name, FString(TEXT("Project")));
        TestEqual(TEXT("plugin root is second"), Roots[1].Name, FString(TEXT("EasyEditorPlugin")));
        TestTrue(TEXT("roots are absolute"), !FPaths::IsRelative(Roots[0].AbsolutePath)
            && !FPaths::IsRelative(Roots[1].AbsolutePath));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsEditorReloadExtensionTest,
    "PuertsEditorPlugin.Environment.ReloadsCommonJsOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsEditorReloadExtensionTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("JavaScript is reloadable"), IsPuertsEditorReloadableSource(TEXT("runtime.js")));
    TestTrue(TEXT("extension comparison is case insensitive"), IsPuertsEditorReloadableSource(TEXT("Runtime.JS")));
    TestFalse(TEXT("MJS is not reloadable"), IsPuertsEditorReloadableSource(TEXT("runtime.mjs")));
    TestFalse(TEXT("CJS is not reloadable"), IsPuertsEditorReloadableSource(TEXT("runtime.cjs")));
    TestFalse(TEXT("package metadata is not reloadable"), IsPuertsEditorReloadableSource(TEXT("package.json")));
    return true;
}

#endif
