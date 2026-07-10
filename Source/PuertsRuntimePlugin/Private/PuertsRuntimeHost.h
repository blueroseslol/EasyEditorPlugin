#pragma once

#include "CoreMinimal.h"
#include "PuertsMultiRootModuleLoader.h"

class UGameInstance;

class IPuertsRuntimeEnvironment
{
public:
    virtual ~IPuertsRuntimeEnvironment() = default;
    virtual bool Start() = 0;
    virtual bool ReloadSource(const FString& AbsolutePath) = 0;
};

using FPuertsRuntimeEnvironmentFactory = TFunction<TUniquePtr<IPuertsRuntimeEnvironment>()>;

class FPuertsRuntimeHost
{
public:
    explicit FPuertsRuntimeHost(FPuertsRuntimeEnvironmentFactory InFactory);

    bool Start();
    void Stop();
    bool Restart();
    bool IsRunning() const { return Environment != nullptr; }
    bool ReloadSource(const FString& AbsolutePath);

private:
    FPuertsRuntimeEnvironmentFactory Factory;
    TUniquePtr<IPuertsRuntimeEnvironment> Environment;
};

struct FPuertsJsEnvironmentConfig
{
    TArray<FPuertsScriptRoot> Roots;
    FString EntryModule;
    int32 DebugPort = -1;
    TWeakObjectPtr<UGameInstance> GameInstance;
    TFunction<void(const FString&)> OnSourceLoaded;
};

TUniquePtr<IPuertsRuntimeEnvironment> CreatePuertsJsEnvironment(FPuertsJsEnvironmentConfig Config);
