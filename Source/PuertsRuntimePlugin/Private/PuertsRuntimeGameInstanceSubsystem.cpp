#include "PuertsRuntimeGameInstanceSubsystem.h"

#include "PuertsRuntimeHost.h"
#include "PuertsRuntimePlugin.h"
#include "PuertsRuntimeSettings.h"

#include "Engine/GameInstance.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

UPuertsRuntimeGameInstanceSubsystem::~UPuertsRuntimeGameInstanceSubsystem() = default;

void UPuertsRuntimeGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const UPuertsRuntimeSettings* Settings = GetDefault<UPuertsRuntimeSettings>();
    FPuertsJsEnvironmentConfig Config;
    Config.Roots = BuildScriptRoots();
    Config.EntryModule = Settings->EntryModule;
    Config.DebugPort = Settings->DebugPort;
    Config.GameInstance = GetGameInstance();

    const TWeakObjectPtr<UPuertsRuntimeGameInstanceSubsystem> WeakThis(this);
    Config.OnSourceLoaded = [WeakThis](const FString& Path) {
        if (UPuertsRuntimeGameInstanceSubsystem* Runtime = WeakThis.Get())
        {
            const FString CanonicalPath = FPuertsMultiRootModuleLoader::CanonicalizePath(Path);
            Runtime->LoadedSourcePaths.Add(CanonicalPath);
            Runtime->SourceLoadedEvent.Broadcast(CanonicalPath);
        }
    };

    RuntimeHost = MakeUnique<FPuertsRuntimeHost>([Config = MoveTemp(Config)]() mutable {
        return CreatePuertsJsEnvironment(Config);
    });

    FPuertsRuntimePluginModule::OnRuntimeInstanceCreated().Broadcast(this);
    if (Settings->bAutoStart)
    {
        StartRuntime();
    }
}

void UPuertsRuntimeGameInstanceSubsystem::Deinitialize()
{
    FPuertsRuntimePluginModule::OnRuntimeInstanceDestroyed().Broadcast(this);
    StopRuntime();
    RuntimeHost.Reset();

    Super::Deinitialize();
}

bool UPuertsRuntimeGameInstanceSubsystem::StartRuntime()
{
    return RuntimeHost && RuntimeHost->Start();
}

void UPuertsRuntimeGameInstanceSubsystem::StopRuntime()
{
    if (RuntimeHost)
    {
        RuntimeHost->Stop();
    }
    LoadedSourcePaths.Reset();
}

bool UPuertsRuntimeGameInstanceSubsystem::RestartRuntime()
{
    LoadedSourcePaths.Reset();
    return RuntimeHost && RuntimeHost->Restart();
}

bool UPuertsRuntimeGameInstanceSubsystem::IsRunning() const
{
    return RuntimeHost && RuntimeHost->IsRunning();
}

bool UPuertsRuntimeGameInstanceSubsystem::ReloadSource(const FString& AbsolutePath)
{
    return RuntimeHost && RuntimeHost->ReloadSource(
        FPuertsMultiRootModuleLoader::CanonicalizePath(AbsolutePath));
}

TArray<FPuertsScriptRoot> UPuertsRuntimeGameInstanceSubsystem::BuildScriptRoots() const
{
    const UPuertsRuntimeSettings* Settings = GetDefault<UPuertsRuntimeSettings>();
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("EasyEditorPlugin"));
    checkf(Plugin.IsValid(), TEXT("EasyEditorPlugin descriptor is not registered"));

    return {
        {TEXT("Project"), FPaths::ProjectContentDir() / Settings->ProjectScriptRoot},
        {TEXT("EasyEditorPlugin"), Plugin->GetContentDir() / Settings->PluginScriptRoot},
    };
}
