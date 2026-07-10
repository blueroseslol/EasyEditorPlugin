#pragma once

#include "CoreMinimal.h"
#include "PuertsMultiRootModuleLoader.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PuertsRuntimeGameInstanceSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FPuertsRuntimeSourceLoaded, const FString&);

class FPuertsRuntimeHost;

UCLASS()
class PUERTSRUNTIMEPLUGIN_API UPuertsRuntimeGameInstanceSubsystem final : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual ~UPuertsRuntimeGameInstanceSubsystem() override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool StartRuntime();
    void StopRuntime();
    bool RestartRuntime();
    bool IsRunning() const;
    bool ReloadSource(const FString& AbsolutePath);

    const TSet<FString>& GetLoadedSourcePaths() const { return LoadedSourcePaths; }
    FPuertsRuntimeSourceLoaded& OnSourceLoaded() { return SourceLoadedEvent; }

private:
    TArray<FPuertsScriptRoot> BuildScriptRoots() const;

    TUniquePtr<FPuertsRuntimeHost> RuntimeHost;
    TSet<FString> LoadedSourcePaths;
    FPuertsRuntimeSourceLoaded SourceLoadedEvent;
};
