#include "PuertsRuntimeHotReloadManager.h"

#include "PuertsMultiRootModuleLoader.h"
#include "PuertsRuntimeGameInstanceSubsystem.h"
#include "PuertsRuntimePlugin.h"

#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "SourceFileWatcher.h"

DEFINE_LOG_CATEGORY_STATIC(LogPuertsRuntimeHotReload, Log, All);

bool IsPuertsRuntimeRestartWorld(EWorldType::Type WorldType)
{
    return WorldType == EWorldType::PIE || WorldType == EWorldType::Game;
}

void FPuertsRuntimeHotReloadManager::Startup()
{
    CreatedHandle = FPuertsRuntimePluginModule::OnRuntimeInstanceCreated().AddRaw(
        this, &FPuertsRuntimeHotReloadManager::Attach);
    DestroyedHandle = FPuertsRuntimePluginModule::OnRuntimeInstanceDestroyed().AddRaw(
        this, &FPuertsRuntimeHotReloadManager::Detach);

    RestartConsoleCommand = MakeUnique<FAutoConsoleCommand>(TEXT("PuertsRuntime.Restart"),
        TEXT("Restart Puerts gameplay runtimes in PIE and game worlds"),
        FConsoleCommandDelegate::CreateRaw(this, &FPuertsRuntimeHotReloadManager::RestartRuntimes));

    AttachExistingRuntimes();
}

void FPuertsRuntimeHotReloadManager::Shutdown()
{
    if (CreatedHandle.IsValid())
    {
        FPuertsRuntimePluginModule::OnRuntimeInstanceCreated().Remove(CreatedHandle);
        CreatedHandle.Reset();
    }
    if (DestroyedHandle.IsValid())
    {
        FPuertsRuntimePluginModule::OnRuntimeInstanceDestroyed().Remove(DestroyedHandle);
        DestroyedHandle.Reset();
    }

    RestartConsoleCommand.Reset();
    for (TPair<TWeakObjectPtr<UPuertsRuntimeGameInstanceSubsystem>, FRuntimeWatcher>& Pair : Watchers)
    {
        if (UPuertsRuntimeGameInstanceSubsystem* Runtime = Pair.Key.Get())
        {
            Runtime->OnSourceLoaded().Remove(Pair.Value.SourceLoadedHandle);
            Runtime->OnSourcesReset().Remove(Pair.Value.SourcesResetHandle);
        }
    }
    Watchers.Empty();
}

void FPuertsRuntimeHotReloadManager::Attach(UPuertsRuntimeGameInstanceSubsystem* Runtime)
{
    if (!Runtime || Watchers.Contains(Runtime))
    {
        return;
    }

    const TWeakObjectPtr<UPuertsRuntimeGameInstanceSubsystem> WeakRuntime(Runtime);
    FRuntimeWatcher RuntimeWatcher;
    RuntimeWatcher.SourcesResetHandle = Runtime->OnSourcesReset().AddRaw(
        this, &FPuertsRuntimeHotReloadManager::ResetWatcher);
    FRuntimeWatcher& StoredWatcher = Watchers.Add(WeakRuntime, MoveTemp(RuntimeWatcher));
    InitializeWatcher(Runtime, StoredWatcher);
}

void FPuertsRuntimeHotReloadManager::InitializeWatcher(
    UPuertsRuntimeGameInstanceSubsystem* Runtime, FRuntimeWatcher& RuntimeWatcher)
{
    const TWeakObjectPtr<UPuertsRuntimeGameInstanceSubsystem> WeakRuntime(Runtime);
    RuntimeWatcher.Watcher = MakeShared<puerts::FSourceFileWatcher>([WeakRuntime](const FString& Path) {
        const FString CanonicalPath = FPuertsMultiRootModuleLoader::CanonicalizePath(Path);
        auto Reload = [WeakRuntime, CanonicalPath]() {
            if (UPuertsRuntimeGameInstanceSubsystem* CurrentRuntime = WeakRuntime.Get())
            {
                CurrentRuntime->ReloadSource(CanonicalPath);
            }
        };

        if (IsInGameThread())
        {
            Reload();
        }
        else
        {
            AsyncTask(ENamedThreads::GameThread, MoveTemp(Reload));
        }
    });

    const TWeakPtr<puerts::FSourceFileWatcher> WeakWatcher = RuntimeWatcher.Watcher;
    RuntimeWatcher.SourceLoadedHandle = Runtime->OnSourceLoaded().AddLambda([WeakWatcher](const FString& Path) {
        if (FPaths::GetExtension(Path).Equals(TEXT("js"), ESearchCase::IgnoreCase))
        {
            if (const TSharedPtr<puerts::FSourceFileWatcher> Watcher = WeakWatcher.Pin())
            {
                Watcher->OnSourceLoaded(FPuertsMultiRootModuleLoader::CanonicalizePath(Path));
            }
        }
    });

    for (const FString& Path : Runtime->GetLoadedSourcePaths())
    {
        if (FPaths::GetExtension(Path).Equals(TEXT("js"), ESearchCase::IgnoreCase))
        {
            RuntimeWatcher.Watcher->OnSourceLoaded(FPuertsMultiRootModuleLoader::CanonicalizePath(Path));
        }
    }
}

void FPuertsRuntimeHotReloadManager::ResetWatcher(UPuertsRuntimeGameInstanceSubsystem* Runtime)
{
    if (FRuntimeWatcher* RuntimeWatcher = Watchers.Find(Runtime))
    {
        Runtime->OnSourceLoaded().Remove(RuntimeWatcher->SourceLoadedHandle);
        RuntimeWatcher->Watcher.Reset();
        InitializeWatcher(Runtime, *RuntimeWatcher);
    }
}

void FPuertsRuntimeHotReloadManager::Detach(UPuertsRuntimeGameInstanceSubsystem* Runtime)
{
    if (!Runtime)
    {
        return;
    }

    if (FRuntimeWatcher* RuntimeWatcher = Watchers.Find(Runtime))
    {
        Runtime->OnSourceLoaded().Remove(RuntimeWatcher->SourceLoadedHandle);
        Runtime->OnSourcesReset().Remove(RuntimeWatcher->SourcesResetHandle);
        Watchers.Remove(Runtime);
    }
}

void FPuertsRuntimeHotReloadManager::AttachExistingRuntimes()
{
    if (!GEngine)
    {
        return;
    }

    TSet<UPuertsRuntimeGameInstanceSubsystem*> ExistingRuntimes;
    for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
    {
        UWorld* World = WorldContext.World();
        UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
        if (GameInstance)
        {
            if (UPuertsRuntimeGameInstanceSubsystem* Runtime =
                    GameInstance->GetSubsystem<UPuertsRuntimeGameInstanceSubsystem>())
            {
                ExistingRuntimes.Add(Runtime);
            }
        }
    }

    for (UPuertsRuntimeGameInstanceSubsystem* Runtime : ExistingRuntimes)
    {
        Attach(Runtime);
    }
}

void FPuertsRuntimeHotReloadManager::RestartRuntimes()
{
    if (!GEngine)
    {
        UE_LOG(LogPuertsRuntimeHotReload, Warning, TEXT("Cannot restart Puerts runtimes without an engine"));
        return;
    }

    TSet<UPuertsRuntimeGameInstanceSubsystem*> Runtimes;
    for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
    {
        if (!IsPuertsRuntimeRestartWorld(WorldContext.WorldType))
        {
            continue;
        }

        UWorld* World = WorldContext.World();
        UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
        if (GameInstance)
        {
            if (UPuertsRuntimeGameInstanceSubsystem* Runtime =
                    GameInstance->GetSubsystem<UPuertsRuntimeGameInstanceSubsystem>())
            {
                Runtimes.Add(Runtime);
            }
        }
    }

    if (Runtimes.IsEmpty())
    {
        UE_LOG(LogPuertsRuntimeHotReload, Warning, TEXT("No active PIE or game Puerts runtime to restart"));
        return;
    }

    for (UPuertsRuntimeGameInstanceSubsystem* Runtime : Runtimes)
    {
        const bool bRestarted = Runtime->RestartRuntime();
        UE_LOG(LogPuertsRuntimeHotReload, Log, TEXT("Puerts runtime %s for GameInstance %s"),
            bRestarted ? TEXT("restarted") : TEXT("failed to restart"),
            *GetNameSafe(Runtime->GetGameInstance()));
    }
}
