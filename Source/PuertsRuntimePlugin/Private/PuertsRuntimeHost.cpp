#include "PuertsRuntimeHost.h"

#include "PuertsRuntimePlugin.h"

#include "Engine/GameInstance.h"
#include "JsEnv.h"
#include "JSLogger.h"
#include "Misc/FileHelper.h"
#include "PString.h"

namespace
{
class FPuertsJsEnvironment final : public IPuertsRuntimeEnvironment
{
public:
    explicit FPuertsJsEnvironment(FPuertsJsEnvironmentConfig Config)
        : EntryModule(MoveTemp(Config.EntryModule))
        , GameInstance(Config.GameInstance)
    {
        const std::shared_ptr<FPuertsMultiRootModuleLoader> ModuleLoader =
            std::make_shared<FPuertsMultiRootModuleLoader>(MoveTemp(Config.Roots));

        FString EntryPath;
        FString EntryDebugPath;
        if (!ModuleLoader->Search(TEXT(""), EntryModule, EntryPath, EntryDebugPath))
        {
            UE_LOG(LogPuertsRuntimePlugin, Error, TEXT("%s"), *ModuleLoader->GetLastSearchDiagnostic());
            return;
        }

        JsEnv = MakeUnique<puerts::FJsEnv>(ModuleLoader, std::make_shared<puerts::FDefaultLogger>(), Config.DebugPort,
            [Callback = MoveTemp(Config.OnSourceLoaded)](const FString& Path) {
                if (Callback)
                {
                    Callback(FPuertsMultiRootModuleLoader::CanonicalizePath(Path));
                }
            });
    }

    virtual bool Start() override
    {
        UGameInstance* GameInstanceObject = GameInstance.Get();
        if (!JsEnv || !GameInstanceObject)
        {
            return false;
        }

        TArray<TPair<FString, UObject*>> Arguments;
        Arguments.Emplace(TEXT("GameInstance"), GameInstanceObject);
        JsEnv->Start(EntryModule, Arguments);
        return true;
    }

    virtual bool ReloadSource(const FString& AbsolutePath) override
    {
        if (!JsEnv)
        {
            return false;
        }

        TArray<uint8> Source;
        if (!FFileHelper::LoadFileToArray(Source, *AbsolutePath))
        {
            UE_LOG(LogPuertsRuntimePlugin, Error, TEXT("Failed to read Puerts source: %s"), *AbsolutePath);
            return false;
        }

        JsEnv->ReloadSource(AbsolutePath, puerts::PString(
            reinterpret_cast<const char*>(Source.GetData()), Source.Num()));
        return true;
    }

private:
    FString EntryModule;
    TWeakObjectPtr<UGameInstance> GameInstance;
    TUniquePtr<puerts::FJsEnv> JsEnv;
};
} // namespace

FPuertsRuntimeHost::FPuertsRuntimeHost(FPuertsRuntimeEnvironmentFactory InFactory)
    : Factory(MoveTemp(InFactory))
{
}

bool FPuertsRuntimeHost::Start()
{
    if (Environment)
    {
        return true;
    }

    if (!Factory)
    {
        return false;
    }

    Environment = Factory();
    if (!Environment || !Environment->Start())
    {
        Environment.Reset();
        return false;
    }

    return true;
}

void FPuertsRuntimeHost::Stop()
{
    Environment.Reset();
}

bool FPuertsRuntimeHost::Restart()
{
    Stop();
    return Start();
}

bool FPuertsRuntimeHost::ReloadSource(const FString& AbsolutePath)
{
    return Environment && Environment->ReloadSource(AbsolutePath);
}

TUniquePtr<IPuertsRuntimeEnvironment> CreatePuertsJsEnvironment(FPuertsJsEnvironmentConfig Config)
{
    return MakeUnique<FPuertsJsEnvironment>(MoveTemp(Config));
}
