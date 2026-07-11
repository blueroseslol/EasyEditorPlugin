#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "HAL/IConsoleManager.h"
#include "UObject/WeakObjectPtr.h"

namespace puerts
{
class FSourceFileWatcher;
}

class UPuertsRuntimeGameInstanceSubsystem;

bool IsPuertsRuntimeRestartWorld(EWorldType::Type WorldType);

class FPuertsRuntimeHotReloadManager
{
public:
    void Startup();
    void Shutdown();

private:
    struct FRuntimeWatcher
    {
        TSharedPtr<puerts::FSourceFileWatcher> Watcher;
        FDelegateHandle SourceLoadedHandle;
        FDelegateHandle SourcesResetHandle;
    };

    void Attach(UPuertsRuntimeGameInstanceSubsystem* Runtime);
    void Detach(UPuertsRuntimeGameInstanceSubsystem* Runtime);
    void AttachExistingRuntimes();
    void ResetWatcher(UPuertsRuntimeGameInstanceSubsystem* Runtime);
    void InitializeWatcher(UPuertsRuntimeGameInstanceSubsystem* Runtime, FRuntimeWatcher& RuntimeWatcher);
    void RestartRuntimes();

    TMap<TWeakObjectPtr<UPuertsRuntimeGameInstanceSubsystem>, FRuntimeWatcher> Watchers;
    FDelegateHandle CreatedHandle;
    FDelegateHandle DestroyedHandle;
    TUniquePtr<FAutoConsoleCommand> RestartConsoleCommand;
};
