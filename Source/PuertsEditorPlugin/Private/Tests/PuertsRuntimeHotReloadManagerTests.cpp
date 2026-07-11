#include "Misc/AutomationTest.h"
#include "PuertsRuntimeHotReloadManager.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsRuntimeRestartWorldFilterTest,
    "PuertsEditorPlugin.RuntimeHotReload.RestartsGameplayWorldsOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsRuntimeRestartWorldFilterTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("PIE world is eligible"), IsPuertsRuntimeRestartWorld(EWorldType::PIE));
    TestTrue(TEXT("game world is eligible"), IsPuertsRuntimeRestartWorld(EWorldType::Game));
    TestFalse(TEXT("editor world is excluded"), IsPuertsRuntimeRestartWorld(EWorldType::Editor));
    TestFalse(TEXT("game preview is excluded"), IsPuertsRuntimeRestartWorld(EWorldType::GamePreview));
    TestFalse(TEXT("inactive world is excluded"), IsPuertsRuntimeRestartWorld(EWorldType::Inactive));
    return true;
}

#endif
