#include "PuertsEditorPlugin.h"

#include "PuertsEditorEnvironment.h"
#include "Binding.hpp"
#include "Containers/Ticker.h"
#include "ContentBrowserItem.h"
#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"
#include "JsEnv.h"
#include "JSLogger.h"
#include "LevelEditor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Object.hpp"
#include "SourceFileWatcher.h"
#include "ToolMenuDelegates.h"
#include "ToolMenus.h"
#include "UECompatible.h"
#include "UEDataBinding.hpp"

#include <functional>
#include <memory>

DEFINE_LOG_CATEGORY_STATIC(LogPuertsEditorPlugin, Log, All);

namespace puerts
{
template <typename T>
struct ScriptTypeName<TAttribute<T>>
{
    static constexpr auto value()
    {
        return ScriptTypeName<T>::value();
    }
};

namespace v8_impl
{
template <typename T>
struct Converter<TAttribute<T>>
{
    static API::ValueType toScript(API::ContextType Context, TAttribute<T> Value)
    {
        return Value.IsSet() ? Converter<T>::toScript(Context, Value.Get()) : API::GetUndefined(Context);
    }

    static TAttribute<T> toCpp(API::ContextType Context, const API::ValueType Value)
    {
        return API::IsNullOrUndefined(Context, Value)
            ? TAttribute<T>()
            : TAttribute<T>(Converter<T>::toCpp(Context, Value));
    }

    static bool accept(API::ContextType Context, const API::ValueType Value)
    {
        return API::IsNullOrUndefined(Context, Value) || Converter<T>::accept(Context, Value);
    }
};
} // namespace v8_impl
} // namespace puerts

namespace
{
constexpr const TCHAR* EditorEntryModule = TEXT("@matrix/puerts-runtime/EditorMain");
constexpr const TCHAR* ToolMenuHandlerName = TEXT("TypeScript");
}

TArray<FPuertsScriptRoot> BuildPuertsEditorScriptRoots()
{
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("EasyEditorPlugin"));
    checkf(Plugin.IsValid(), TEXT("EasyEditorPlugin descriptor is not registered"));

    return {
        {TEXT("Project"), FPuertsMultiRootModuleLoader::CanonicalizePath(FPaths::ProjectContentDir() / TEXT("JavaScript"))},
        {TEXT("EasyEditorPlugin"), FPuertsMultiRootModuleLoader::CanonicalizePath(Plugin->GetContentDir() / TEXT("JavaScript"))},
    };
}

bool IsPuertsEditorReloadableSource(const FString& Path)
{
    return FPaths::GetExtension(Path).Equals(TEXT("js"), ESearchCase::IgnoreCase);
}

class FPuertsEditorPluginState
{
public:
    void Startup()
    {
        char GCFlags[] = "--expose-gc";
        v8::V8::SetFlagsFromString(GCFlags, sizeof(GCFlags));
        PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FPuertsEditorPluginState::OnPostEngineInit);
    }

    void Shutdown()
    {
        if (PostEngineInitHandle.IsValid())
        {
            FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
            PostEngineInitHandle.Reset();
        }

        if (MapChangedHandle.IsValid())
        {
            if (FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
            {
                LevelEditor->OnMapChanged().Remove(MapChangedHandle);
            }
            MapChangedHandle.Reset();
        }

        if (TickerHandle.IsValid())
        {
            FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
            TickerHandle.Reset();
        }

        if (bToolMenuHandlerRegistered)
        {
            if (UToolMenus* ToolMenus = UToolMenus::TryGet())
            {
                ToolMenus->UnregisterStringCommandHandler(ToolMenuHandlerName);
            }
            bToolMenuHandlerRegistered = false;
        }

        RestartConsoleCommand.Reset();
        UnInitJsEnv();
    }

    std::function<void()> OnJsEnvPreReload;
    std::function<void(const FString&)> Eval;
    TSparseArray<TUniquePtr<FAutoConsoleCommand>> TsConsoleCommands;
    FSimpleMulticastDelegate OnJsEnvCleanup;

    bool Restart()
    {
        if (OnJsEnvPreReload)
        {
            OnJsEnvPreReload();
        }

        UnInitJsEnv();
        InitJsEnv();
        bStartupScriptCalled = true;
        return StartEditorScript();
    }

private:
    void OnPostEngineInit()
    {
        if (bPostEngineInitialized)
        {
            return;
        }
        bPostEngineInitialized = true;

        FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
        MapChangedHandle = LevelEditor.OnMapChanged().AddRaw(this, &FPuertsEditorPluginState::HandleMapChanged);

        InitJsEnv();
        TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateRaw(this, &FPuertsEditorPluginState::Tick));

        RestartConsoleCommand = MakeUnique<FAutoConsoleCommand>(TEXT("PuertsEditor.Restart"),
            TEXT("Restart the Puerts editor script environment"),
            FConsoleCommandDelegate::CreateRaw(this, &FPuertsEditorPluginState::RestartFromConsole));

        UToolMenus::Get()->RegisterStringCommandHandler(ToolMenuHandlerName,
            FToolMenuExecuteString::CreateRaw(this, &FPuertsEditorPluginState::ExecuteStringCommand));
        bToolMenuHandlerRegistered = true;
    }

    void InitJsEnv()
    {
        ModuleLoader = std::make_shared<FPuertsMultiRootModuleLoader>(BuildPuertsEditorScriptRoots());
        SourceFileWatcher = std::make_shared<puerts::FSourceFileWatcher>([this](const FString& Path) {
            if (!JsEnv || !IsPuertsEditorReloadableSource(Path))
            {
                return;
            }

            TArray<uint8> Source;
            if (FFileHelper::LoadFileToArray(Source, *Path))
            {
                JsEnv->ReloadSource(Path,
                    puerts::PString(reinterpret_cast<const char*>(Source.GetData()), Source.Num()));
            }
            else
            {
                UE_LOG(LogPuertsEditorPlugin, Error, TEXT("Failed to read Puerts editor source: %s"), *Path);
            }
        });

        JsEnv = std::make_shared<puerts::FJsEnv>(ModuleLoader, std::make_shared<puerts::FDefaultLogger>(), -1,
            [this](const FString& Path) {
                if (SourceFileWatcher && IsPuertsEditorReloadableSource(Path))
                {
                    SourceFileWatcher->OnSourceLoaded(FPuertsMultiRootModuleLoader::CanonicalizePath(Path));
                }
            });
    }

    void UnInitJsEnv()
    {
        OnJsEnvCleanup.Broadcast();
        Eval = nullptr;
        OnJsEnvPreReload = nullptr;
        TsConsoleCommands.Empty();
        JsEnv.reset();
        SourceFileWatcher.reset();
        ModuleLoader.reset();
    }

    bool StartEditorScript()
    {
        if (!JsEnv || !ModuleLoader)
        {
            return false;
        }

        FString EntryPath;
        FString EntryAbsolutePath;
        if (!ModuleLoader->Search(TEXT(""), EditorEntryModule, EntryPath, EntryAbsolutePath))
        {
            UE_LOG(LogPuertsEditorPlugin, Warning, TEXT("Puerts editor entry was not started: %s"),
                *ModuleLoader->GetLastSearchDiagnostic());
            return false;
        }

        JsEnv->Start(EditorEntryModule);
        UE_LOG(LogPuertsEditorPlugin, Log, TEXT("Started Puerts editor entry: %s"), EditorEntryModule);
        return true;
    }

    bool Tick(float DeltaTime)
    {
        bStartupScriptCalled = true;
        TickerHandle.Reset();
        StartEditorScript();
        return false;
    }

    void RestartFromConsole()
    {
        if (!Restart())
        {
            UE_LOG(LogPuertsEditorPlugin, Warning, TEXT("Puerts editor environment restarted without an entry script"));
        }
    }

    void ExecuteStringCommand(const FString& Command, const FToolMenuContext& Context)
    {
        if (Eval)
        {
            Eval(Command);
        }
        else
        {
            UE_LOG(LogPuertsEditorPlugin, Warning,
                TEXT("Call EasyEditorPlugin.SetEval before executing a TypeScript string command"));
        }
    }

    void HandleMapChanged(UWorld* World, EMapChangeType MapChangeType)
    {
        if (MapChangeType == EMapChangeType::TearDownWorld && IsInGameThread() && JsEnv)
        {
            JsEnv->RequestFullGarbageCollectionForTesting();
        }
    }

    std::shared_ptr<FPuertsMultiRootModuleLoader> ModuleLoader;
    std::shared_ptr<puerts::FJsEnv> JsEnv;
    std::shared_ptr<puerts::FSourceFileWatcher> SourceFileWatcher;
    TUniquePtr<FAutoConsoleCommand> RestartConsoleCommand;
    FDelegateHandle PostEngineInitHandle;
    FDelegateHandle MapChangedHandle;
    FTSTicker::FDelegateHandle TickerHandle;
    bool bPostEngineInitialized = false;
    bool bStartupScriptCalled = false;
    bool bToolMenuHandlerRegistered = false;
};

struct EasyEditorPlugin
{
    static FPuertsEditorPluginState& State()
    {
        return FModuleManager::LoadModuleChecked<FPuertsEditorPluginModule>(TEXT("PuertsEditorPlugin")).GetState();
    }

    static void SetOnJsEnvPreReload(std::function<void()> Func)
    {
        State().OnJsEnvPreReload = MoveTemp(Func);
    }

    static void SetEval(std::function<void(const FString&)> Func)
    {
        State().Eval = MoveTemp(Func);
    }

    static int32 AddConsoleCommand(const TCHAR* Name, const TCHAR* Help, puerts::Function Command)
    {
        return State().TsConsoleCommands.Add(MakeUnique<FAutoConsoleCommand>(Name, Help,
            FConsoleCommandWithArgsDelegate::CreateLambda([Command](const TArray<FString>& Args) {
                v8::Isolate* Isolate = Command.Isolate;
                v8::Isolate::Scope IsolateScope(Isolate);
                v8::HandleScope HandleScope(Isolate);
                const v8::Local<v8::Context> Context = Command.GContext.Get(Isolate);
                v8::Context::Scope ContextScope(Context);
                const v8::Local<v8::Function> JsFunction = Command.GObject.Get(Isolate).As<v8::Function>();
                v8::TryCatch TryCatch(Isolate);

                v8::Local<v8::Value>* JsArgs = static_cast<v8::Local<v8::Value>*>(
                    FMemory_Alloca(sizeof(v8::Local<v8::Value>) * Args.Num()));
                for (int32 Index = 0; Index < Args.Num(); ++Index)
                {
                    JsArgs[Index] = puerts::FV8Utils::ToV8String(Isolate, Args[Index]);
                }

                JsFunction->Call(Context, v8::Undefined(Isolate), Args.Num(), JsArgs);
                if (TryCatch.HasCaught())
                {
                    UE_LOG(Puerts, Error, TEXT("Puerts console command threw: %s"),
                        *puerts::FV8Utils::TryCatchToString(Isolate, &TryCatch));
                }
            })));
    }

    static void RemoveConsoleCommand(int32 Index)
    {
        State().TsConsoleCommands.RemoveAt(Index);
    }
};

UsingUStruct(FToolMenuEntry);
UsingUClass(UToolMenu);
UsingUStruct(FToolMenuContext);
UsingUStruct(FContentBrowserItem);
UsingCppType(FSlateIcon);
UsingCppType(EasyEditorPlugin);

static FToolMenuEntry FToolMenuEntry_InitMenuEntry(const FName Name, const FText& Label, const FText& ToolTip,
    std::function<void(const FToolMenuContext&)> ExecuteAction)
{
    return FToolMenuEntry::InitMenuEntry(Name, Label, ToolTip, TAttribute<FSlateIcon>(),
        FToolUIActionChoice(FToolMenuExecuteAction::CreateLambda([ExecuteAction](const FToolMenuContext& Context) {
            if (ExecuteAction)
            {
                ExecuteAction(Context);
            }
        })));
}

static FToolMenuEntry FToolMenuEntry_InitToolBarButton(const FName Name, const FText& Label,
    std::function<void(const FToolMenuContext&)> ExecuteAction)
{
    return FToolMenuEntry::InitToolBarButton(Name,
        FToolUIActionChoice(FToolMenuExecuteAction::CreateLambda([ExecuteAction](const FToolMenuContext& Context) {
            if (ExecuteAction)
            {
                ExecuteAction(Context);
            }
        })), Label);
}

static FToolMenuEntry FToolMenuEntry_InitComboButton(const FName Name,
    std::function<void(const FToolMenuContext&)> ExecuteAction, std::function<void(UToolMenu*)> MenuContentGenerator,
    const TAttribute<FText>& Label = TAttribute<FText>(), const TAttribute<FText>& ToolTip = TAttribute<FText>(),
    const FSlateIcon& Icon = FSlateIcon())
{
    return FToolMenuEntry::InitComboButton(Name,
        FToolUIActionChoice(FToolMenuExecuteAction::CreateLambda([ExecuteAction](const FToolMenuContext& Context) {
            if (ExecuteAction)
            {
                ExecuteAction(Context);
            }
        })),
        FNewToolMenuDelegate::CreateLambda([MenuContentGenerator](UToolMenu* SubMenu) {
            if (MenuContentGenerator)
            {
                MenuContentGenerator(SubMenu);
            }
        }), Label, ToolTip, Icon);
}

struct FAutoRegisterForPuertsEditorPlugin
{
    FAutoRegisterForPuertsEditorPlugin()
    {
        puerts::DefineClass<FSlateIcon>()
            .Constructor(CombineConstructors(MakeConstructor(FSlateIcon),
                MakeConstructor(FSlateIcon, const FName&, const FName&),
                MakeConstructor(FSlateIcon, const FName&, const FName&, const FName&)))
            .Register();

        puerts::DefineClass<FToolMenuEntry>()
            .Function("InitMenuEntry", MakeFunction(&FToolMenuEntry_InitMenuEntry))
            .Function("InitToolBarButton", MakeFunction(&FToolMenuEntry_InitToolBarButton))
            .Function("InitComboButton", MakeFunction(&FToolMenuEntry_InitComboButton,
                TAttribute<FText>(), TAttribute<FText>(), FSlateIcon()))
            .Register();

        puerts::DefineClass<EasyEditorPlugin>()
            .Function("SetOnJsEnvPreReload", MakeFunction(&EasyEditorPlugin::SetOnJsEnvPreReload))
            .Function("SetEval", MakeFunction(&EasyEditorPlugin::SetEval))
            .Function("AddConsoleCommand", MakeFunction(&EasyEditorPlugin::AddConsoleCommand))
            .Function("RemoveConsoleCommand", MakeFunction(&EasyEditorPlugin::RemoveConsoleCommand))
            .Register();

        puerts::DefineClass<FContentBrowserItem>()
            .Method("IsFolder", MakeFunction(&FContentBrowserItem::IsFolder))
            .Method("IsFile", MakeFunction(&FContentBrowserItem::IsFile))
            .Method("GetItemName", MakeFunction(&FContentBrowserItem::GetItemName))
            .Method("GetItemPhysicalPath", MakeFunction(&FContentBrowserItem::GetItemPhysicalPath))
            .Register();
    }
};

FAutoRegisterForPuertsEditorPlugin GAutoRegisterForPuertsEditorPlugin;

FPuertsEditorPluginModule::FPuertsEditorPluginModule() = default;
FPuertsEditorPluginModule::~FPuertsEditorPluginModule() = default;

void FPuertsEditorPluginModule::StartupModule()
{
    State = MakeUnique<FPuertsEditorPluginState>();
    State->Startup();
}

void FPuertsEditorPluginModule::ShutdownModule()
{
    if (State)
    {
        State->Shutdown();
        State.Reset();
    }
}

FPuertsEditorPluginState& FPuertsEditorPluginModule::GetState()
{
    check(State);
    return *State;
}

IMPLEMENT_MODULE(FPuertsEditorPluginModule, PuertsEditorPlugin)
