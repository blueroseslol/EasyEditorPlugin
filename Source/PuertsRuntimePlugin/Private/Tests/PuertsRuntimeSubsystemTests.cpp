#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "PuertsRuntimeSettings.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsRuntimeSettingsDefaultsTest, "PuertsRuntimePlugin.Settings.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsRuntimeSettingsDefaultsTest::RunTest(const FString& Parameters)
{
    const UPuertsRuntimeSettings* Settings = GetDefault<UPuertsRuntimeSettings>();
    TestTrue(TEXT("auto start"), Settings->bAutoStart);
    TestEqual(TEXT("entry"), Settings->EntryModule, FString(TEXT("Main")));
    TestEqual(TEXT("project root"), Settings->ProjectScriptRoot, FString(TEXT("JavaScript")));
    TestEqual(TEXT("plugin root"), Settings->PluginScriptRoot, FString(TEXT("JavaScript")));
    TestEqual(TEXT("debug disabled"), Settings->DebugPort, -1);
    return true;
}

#endif
