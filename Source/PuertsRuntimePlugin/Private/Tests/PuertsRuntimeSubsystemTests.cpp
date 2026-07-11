#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "PuertsRuntimeSettings.h"
#include "PuertsRuntimeHost.h"
#include "PuertsRuntimeGameInstanceSubsystem.h"

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

namespace
{
class FFakePuertsRuntimeEnvironment final : public IPuertsRuntimeEnvironment
{
public:
    FFakePuertsRuntimeEnvironment(
        int32& InStartCount, int32& InReloadCount, int32& InDestroyCount, const bool bInStartResult)
        : StartCount(InStartCount)
        , ReloadCount(InReloadCount)
        , DestroyCount(InDestroyCount)
        , bStartResult(bInStartResult)
    {
    }

    virtual ~FFakePuertsRuntimeEnvironment() override { ++DestroyCount; }

    virtual bool Start() override
    {
        ++StartCount;
        return bStartResult;
    }

    virtual bool ReloadSource(const FString& AbsolutePath) override
    {
        ++ReloadCount;
        return true;
    }

private:
    int32& StartCount;
    int32& ReloadCount;
    int32& DestroyCount;
    bool bStartResult;
};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsRuntimeHostLifecycleTest, "PuertsRuntimePlugin.Subsystem.HostLifecycleIsIdempotent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsRuntimeHostLifecycleTest::RunTest(const FString& Parameters)
{
    int32 CreateCount = 0;
    int32 StartCount = 0;
    int32 ReloadCount = 0;
    int32 DestroyCount = 0;
    FPuertsRuntimeHost Host([&]() {
        ++CreateCount;
        return MakeUnique<FFakePuertsRuntimeEnvironment>(StartCount, ReloadCount, DestroyCount, true);
    });

    TestFalse(TEXT("reload is rejected while stopped"), Host.ReloadSource(TEXT("not-loaded.js")));
    TestTrue(TEXT("first start succeeds"), Host.Start());
    TestTrue(TEXT("second start is idempotent"), Host.Start());
    TestEqual(TEXT("one environment created"), CreateCount, 1);
    TestEqual(TEXT("one environment start"), StartCount, 1);
    TestTrue(TEXT("reload is forwarded while running"), Host.ReloadSource(TEXT("loaded.js")));
    TestEqual(TEXT("one reload"), ReloadCount, 1);

    TestTrue(TEXT("restart succeeds"), Host.Restart());
    TestEqual(TEXT("restart creates a new environment"), CreateCount, 2);
    TestEqual(TEXT("restart starts the new environment"), StartCount, 2);
    TestEqual(TEXT("restart destroys the old environment"), DestroyCount, 1);

    Host.Stop();
    Host.Stop();
    TestFalse(TEXT("host is stopped"), Host.IsRunning());
    TestEqual(TEXT("final stop destroys once"), DestroyCount, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsRuntimeHostStartFailureTest, "PuertsRuntimePlugin.Subsystem.StartFailureCleansUp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsRuntimeHostStartFailureTest::RunTest(const FString& Parameters)
{
    int32 StartCount = 0;
    int32 ReloadCount = 0;
    int32 DestroyCount = 0;
    FPuertsRuntimeHost Host([&]() {
        return MakeUnique<FFakePuertsRuntimeEnvironment>(StartCount, ReloadCount, DestroyCount, false);
    });

    TestFalse(TEXT("start failure is reported"), Host.Start());
    TestFalse(TEXT("failed environment is not retained"), Host.IsRunning());
    TestEqual(TEXT("start attempted once"), StartCount, 1);
    TestEqual(TEXT("failed environment destroyed once"), DestroyCount, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsRuntimeSubsystemTypeTest, "PuertsRuntimePlugin.Subsystem.IsGameInstanceScoped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsRuntimeSubsystemTypeTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("runtime subsystem is game instance scoped"),
        UPuertsRuntimeGameInstanceSubsystem::StaticClass()->IsChildOf(UGameInstanceSubsystem::StaticClass()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsRuntimeSourcesResetTest, "PuertsRuntimePlugin.Subsystem.StopAndRestartResetSources",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsRuntimeSourcesResetTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UPuertsRuntimeGameInstanceSubsystem* Runtime =
        NewObject<UPuertsRuntimeGameInstanceSubsystem>(GameInstance);
    int32 ResetCount = 0;
    Runtime->OnSourcesReset().AddLambda([&ResetCount](UPuertsRuntimeGameInstanceSubsystem*) { ++ResetCount; });

    Runtime->StopRuntime();
    TestEqual(TEXT("stop resets watched sources"), ResetCount, 1);
    TestFalse(TEXT("restart without a host fails"), Runtime->RestartRuntime());
    TestEqual(TEXT("restart resets watched sources"), ResetCount, 2);
    return true;
}

#endif
