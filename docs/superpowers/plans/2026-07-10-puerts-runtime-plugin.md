# Puerts Runtime Plugin Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 EasyEditorPlugin 拆分为 `PuertsRuntimePlugin` 与 `PuertsEditorPlugin`，让项目入口能够从插件目录加载可复用 gameplay TypeScript 编译产物，并可在 PIE、Standalone 和打包游戏中运行。

**Architecture:** `UPuertsRuntimeGameInstanceSubsystem` 为每个 game instance 持有一个 gameplay `FJsEnv`，`FPuertsMultiRootModuleLoader` 按“项目优先、插件回退”解析模块。编辑器模块保留原有工具 VM 与绑定，并为 gameplay VM 提供热重载和重启支持；插件 TypeScript 独立编译为 `@matrix/puerts-runtime` 包。

**Tech Stack:** Unreal Engine 5.7 C++、Puerts `JsEnv`、TypeScript/CommonJS、Unreal Automation Tests、AutomationTool BuildCookRun。

---

## 文件结构

```text
EasyEditorPlugin/
  EasyEditorPlugin.uplugin
  Source/
    PuertsRuntimePlugin/
      PuertsRuntimePlugin.Build.cs
      Public/PuertsRuntimePlugin.h
      Public/PuertsRuntimeSettings.h
      Public/PuertsRuntimeGameInstanceSubsystem.h
      Public/PuertsMultiRootModuleLoader.h
      Private/PuertsRuntimePlugin.cpp
      Private/PuertsRuntimeSettings.cpp
      Private/PuertsRuntimeLogger.h
      Private/PuertsRuntimeLogger.cpp
      Private/PuertsRuntimeHost.h
      Private/PuertsRuntimeHost.cpp
      Private/PuertsRuntimeGameInstanceSubsystem.cpp
      Private/PuertsMultiRootModuleLoader.cpp
      Private/Tests/PuertsMultiRootModuleLoaderTests.cpp
      Private/Tests/PuertsRuntimeSubsystemTests.cpp
    PuertsEditorPlugin/
      PuertsEditorPlugin.Build.cs
      Public/PuertsEditorPlugin.h
      Private/PuertsEditorPlugin.cpp
      Private/PuertsRuntimeHotReloadManager.h
      Private/PuertsRuntimeHotReloadManager.cpp
  TypeScript/
    package.json
    tsconfig.json
    scripts/write-runtime-package.mjs
    src/index.ts
    src/runtime.ts
    src/EditorMain.ts
  Content/JavaScript/node_modules/@matrix/puerts-runtime/
  Typing/@matrix/puerts-runtime/
```

`PuertsMultiRootModuleLoader` 只负责文件解析和读取；`PuertsRuntimeGameInstanceSubsystem` 只负责 gameplay VM 生命周期；`PuertsRuntimeHotReloadManager` 只负责编辑器 watcher 与 runtime instance 的连接。原静态编辑器绑定继续位于 `PuertsEditorPlugin.cpp`，不进入 runtime target。

## 执行前准备

执行计划时先用 `superpowers:using-git-worktrees` 为 `D:\MatrixTA\EasyEditorPlugin` 创建隔离 worktree。Lyra 只作为 UE 5.7 编译和打包宿主；在 Lyra 的 `.git/info/exclude` 中本地排除 `Plugins/EasyEditorPlugin/` 与 `Plugins/Puerts/`，然后创建以下 junction，二者都不提交到 Lyra：

```powershell
New-Item -ItemType Junction -Path D:\MatrixTA\LyraRPGGameplayAbility\Plugins\EasyEditorPlugin -Target D:\MatrixTA\EasyEditorPlugin
New-Item -ItemType Junction -Path D:\MatrixTA\LyraRPGGameplayAbility\Plugins\Puerts -Target D:\MatrixTA\puerts\unreal\Puerts
```

首次编译前在 `LyraStarterGame.uproject` 启用 `EasyEditorPlugin`；其 `.uplugin` 依赖会同时启用 Puerts。若执行使用的是隔离 worktree，第一条 junction 的 target 必须改为该 worktree 的绝对路径。

### Task 1: 建立双模块骨架

**Files:**
- Modify: `EasyEditorPlugin.uplugin`
- Create: `Source/PuertsRuntimePlugin/PuertsRuntimePlugin.Build.cs`
- Create: `Source/PuertsRuntimePlugin/Public/PuertsRuntimePlugin.h`
- Create: `Source/PuertsRuntimePlugin/Private/PuertsRuntimePlugin.cpp`
- Create: `Source/PuertsEditorPlugin/PuertsEditorPlugin.Build.cs`
- Create: `Source/PuertsEditorPlugin/Public/PuertsEditorPlugin.h`
- Create: `Source/PuertsEditorPlugin/Private/PuertsEditorPlugin.cpp`

- [ ] **Step 1: 将插件描述符改为两个模块**

```json
"Modules": [
  {
    "Name": "PuertsRuntimePlugin",
    "Type": "Runtime",
    "LoadingPhase": "Default"
  },
  {
    "Name": "PuertsEditorPlugin",
    "Type": "Editor",
    "LoadingPhase": "Default"
  }
]
```

- [ ] **Step 2: 创建 runtime Build.cs**

```csharp
using UnrealBuildTool;

public class PuertsRuntimePlugin : ModuleRules
{
    public PuertsRuntimePlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "DeveloperSettings", "JsEnv"
        });
        PrivateDependencyModuleNames.AddRange(new[] { "Projects" });
        bEnableUndefinedIdentifierWarnings = false;
    }
}
```

- [ ] **Step 3: 创建 editor Build.cs**

```csharp
using UnrealBuildTool;

public class PuertsEditorPlugin : ModuleRules
{
    public PuertsEditorPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "PuertsRuntimePlugin" });
        PrivateDependencyModuleNames.AddRange(new[] {
            "CoreUObject", "Engine", "Slate", "SlateCore", "ToolMenus", "UnrealEd",
            "LevelEditor", "ContentBrowserData", "Projects", "JsEnv"
        });
        bEnableUndefinedIdentifierWarnings = false;
    }
}
```

- [ ] **Step 4: 创建可编译的模块入口**

```cpp
// Source/PuertsRuntimePlugin/Public/PuertsRuntimePlugin.h
#pragma once
#include "Modules/ModuleManager.h"

class PUERTSRUNTIMEPLUGIN_API FPuertsRuntimePluginModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
```

```cpp
// Source/PuertsRuntimePlugin/Private/PuertsRuntimePlugin.cpp
#include "PuertsRuntimePlugin.h"

void FPuertsRuntimePluginModule::StartupModule() {}
void FPuertsRuntimePluginModule::ShutdownModule() {}

IMPLEMENT_MODULE(FPuertsRuntimePluginModule, PuertsRuntimePlugin)
```

```cpp
// Source/PuertsEditorPlugin/Public/PuertsEditorPlugin.h
#pragma once
#include "Modules/ModuleManager.h"

class PUERTSEDITORPLUGIN_API FPuertsEditorPluginModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
```

```cpp
// Source/PuertsEditorPlugin/Private/PuertsEditorPlugin.cpp
#include "PuertsEditorPlugin.h"

void FPuertsEditorPluginModule::StartupModule() {}
void FPuertsEditorPluginModule::ShutdownModule() {}

IMPLEMENT_MODULE(FPuertsEditorPluginModule, PuertsEditorPlugin)
```

- [ ] **Step 5: 编译双模块骨架**

Run:

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Build\BatchFiles\Build.bat LyraEditor Win64 Development D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -WaitMutex
```

Expected: `Result: Succeeded`，日志中同时编译 `PuertsRuntimePlugin` 和 `PuertsEditorPlugin`。

- [ ] **Step 6: 提交模块骨架**

```powershell
git add EasyEditorPlugin.uplugin Source/PuertsRuntimePlugin Source/PuertsEditorPlugin
git commit -m "refactor: split puerts runtime and editor modules"
```

### Task 2: 用测试驱动实现多根模块加载器

**Files:**
- Create: `Source/PuertsRuntimePlugin/Public/PuertsMultiRootModuleLoader.h`
- Create: `Source/PuertsRuntimePlugin/Private/PuertsMultiRootModuleLoader.cpp`
- Create: `Source/PuertsRuntimePlugin/Private/Tests/PuertsMultiRootModuleLoaderTests.cpp`

- [ ] **Step 1: 定义加载器接口**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "JSModuleLoader.h"

struct PUERTSRUNTIMEPLUGIN_API FPuertsScriptRoot
{
    FString Name;
    FString AbsolutePath;
};

class PUERTSRUNTIMEPLUGIN_API FPuertsMultiRootModuleLoader final : public puerts::IJSModuleLoader
{
public:
    explicit FPuertsMultiRootModuleLoader(TArray<FPuertsScriptRoot> InRoots);
    virtual bool Search(const FString& RequiredDir, const FString& RequiredModule,
        FString& Path, FString& AbsolutePath) override;
    virtual bool Load(const FString& Path, TArray<uint8>& Content) override;
    virtual FString& GetScriptRoot() override;

    const TArray<FPuertsScriptRoot>& GetRoots() const { return Roots; }
    const FString& GetLastSearchDiagnostic() const { return LastSearchDiagnostic; }
    static FString CanonicalizePath(const FString& Path);

private:
    bool SearchFromDirectory(const FString& Directory, const FString& RequiredModule,
        bool bSearchNodeModules, FString& Path, FString& AbsolutePath) const;
    bool SearchNodeModulesUpward(const FString& RequiredDir, const FString& RequiredModule,
        FString& Path, FString& AbsolutePath) const;
    bool CheckCandidate(const FString& Candidate, FString& Path, FString& AbsolutePath) const;

    TArray<FPuertsScriptRoot> Roots;
    FString ScriptRoot = TEXT("JavaScript");
    FString LastSearchDiagnostic;
};
```

- [ ] **Step 2: 写项目覆盖、插件回退和相对导入测试**

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PuertsMultiRootModuleLoader.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsRootPriorityTest,
    "PuertsRuntimePlugin.Loader.ProjectOverridesPlugin",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPuertsRootPriorityTest::RunTest(const FString& Parameters)
{
    const FString Base = FPaths::CreateTempFilename(*FPaths::ProjectIntermediateDir(), TEXT("PuertsRoots"), TEXT(""));
    const FString ProjectRoot = Base / TEXT("Project");
    const FString PluginRoot = Base / TEXT("Plugin");
    IFileManager::Get().MakeDirectory(*(ProjectRoot / TEXT("node_modules/pkg")), true);
    IFileManager::Get().MakeDirectory(*(PluginRoot / TEXT("node_modules/pkg")), true);
    FFileHelper::SaveStringToFile(TEXT("project"), *(ProjectRoot / TEXT("node_modules/pkg/index.js")));
    FFileHelper::SaveStringToFile(TEXT("plugin"), *(PluginRoot / TEXT("node_modules/pkg/index.js")));

    FPuertsMultiRootModuleLoader Loader({
        {TEXT("Project"), ProjectRoot}, {TEXT("Plugin"), PluginRoot}
    });
    FString Path;
    FString DebugPath;
    TestTrue(TEXT("bare package resolves"), Loader.Search(TEXT(""), TEXT("pkg"), Path, DebugPath));
    TestTrue(TEXT("project root wins"), Path.StartsWith(ProjectRoot));

    IFileManager::Get().DeleteDirectory(*Base, false, true);
    return true;
}
#endif
```

在同一文件增加以下独立测试，每个测试使用独立临时目录并在返回前删除目录：

- `PluginFallback`：项目没有包、插件存在包，断言命中插件。
- `RelativeImportStaysInPackage`：`RequiredDir` 指向插件包，项目有同名文件，断言仍命中插件相对文件。
- `MissingRelativeDoesNotCrossRoots`：插件相对文件缺失、项目有同名文件，断言返回 false。
- `ScopedPackageIsBareModule`：`@matrix/puerts-runtime` 断言可从根目录 `node_modules` 命中。
- `NodeModulesWalksUpward`：从包内嵌套目录请求裸模块，断言向上命中最近 `node_modules`。
- `ExplicitJsonAndPackageResolution`：断言 `config.json`、带 `main` 的 package 和无 `main` 的 `index.js`；不允许把 `config` 隐式解析为 `config.json`。
- `CanonicalPathsMatch`：断言 `Path == AbsolutePath`、`FPaths::IsRelative(Path) == false` 且不含反斜杠。
- `MissingModuleDiagnosticListsRoots`：断言诊断包含请求名、发起目录和两个根名称。

- [ ] **Step 3: 运行测试并确认失败**

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests PuertsRuntimePlugin.Loader;Quit" -TestExit="Automation Test Queue Empty" -log
```

Expected: 编译或测试失败，原因是加载器尚未实现搜索逻辑。

- [ ] **Step 4: 实现与 Puerts 默认加载器一致的候选顺序**

`SearchFromDirectory` 必须复刻上游候选顺序：显式 `.js/.mjs/.cjs/.json` 直接检查；无显式扩展名时尝试 `.js/.mjs/.cjs`，`WITH_V8_BYTECODE` 下再尝试 `.mbc/.cbc`，最后是 `package.json` 与 `index.js`。不隐式尝试 `module.json`。`package.json` 路径直接返回给 Puerts，由上游读取 `main`。只使用 `FPaths::ConvertRelativePathToFull`、`FPaths::NormalizeFilename`、`IPlatformFile::FileExists` 和 `IFileHandle::Read`，不修改 Puerts 上游代码。

```cpp
bool FPuertsMultiRootModuleLoader::Search(const FString& RequiredDir, const FString& RequiredModule,
    FString& Path, FString& AbsolutePath)
{
    const bool bRelativeRequest = RequiredModule.StartsWith(TEXT("./")) ||
        RequiredModule.StartsWith(TEXT("../")) || RequiredModule.StartsWith(TEXT(".\\")) ||
        RequiredModule.StartsWith(TEXT("..\\"));
    const bool bAbsoluteRequest = !FPaths::IsRelative(RequiredModule);

    if (bRelativeRequest)
    {
        return !RequiredDir.IsEmpty() &&
            SearchFromDirectory(RequiredDir, RequiredModule, false, Path, AbsolutePath);
    }
    if (bAbsoluteRequest)
    {
        return SearchFromDirectory(TEXT(""), RequiredModule, false, Path, AbsolutePath);
    }
    if (!RequiredDir.IsEmpty() &&
        (SearchFromDirectory(RequiredDir, RequiredModule, true, Path, AbsolutePath) ||
         SearchNodeModulesUpward(RequiredDir, RequiredModule, Path, AbsolutePath)))
    {
        return true;
    }

    for (const FPuertsScriptRoot& Root : Roots)
    {
        if (SearchFromDirectory(Root.AbsolutePath, RequiredModule, true, Path, AbsolutePath))
        {
            return true;
        }
    }

    LastSearchDiagnostic = FString::Printf(TEXT("Cannot resolve '%s' from '%s'. Roots: %s"),
        *RequiredModule, *RequiredDir,
        *FString::JoinBy(Roots, TEXT(", "), [](const FPuertsScriptRoot& Root) {
            return Root.Name + TEXT("=") + Root.AbsolutePath;
        }));
    UE_LOG(LogTemp, Error, TEXT("%s"), *LastSearchDiagnostic);
    return false;
}
```

`CheckCandidate` 成功时执行：

```cpp
const FString CanonicalPath = CanonicalizePath(Candidate);
Path = CanonicalPath;
AbsolutePath = CanonicalPath;
```

runtime source-loaded、editor source-loaded、watcher change 与 `ReloadSource` 在比较或传递路径前都调用同一个 `CanonicalizePath`。

- [ ] **Step 5: 运行加载器测试**

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests PuertsRuntimePlugin.Loader;Quit" -TestExit="Automation Test Queue Empty" -log
```

Expected: 九个 `PuertsRuntimePlugin.Loader` 测试全部 `Success`。

- [ ] **Step 6: 提交加载器**

```powershell
git add Source/PuertsRuntimePlugin/Public/PuertsMultiRootModuleLoader.h Source/PuertsRuntimePlugin/Private/PuertsMultiRootModuleLoader.cpp Source/PuertsRuntimePlugin/Private/Tests/PuertsMultiRootModuleLoaderTests.cpp
git commit -m "feat: add multi-root puerts module loader"
```

### Task 3: 添加 runtime 设置与脚本根构建

**Files:**
- Create: `Source/PuertsRuntimePlugin/Public/PuertsRuntimeSettings.h`
- Create: `Source/PuertsRuntimePlugin/Private/PuertsRuntimeSettings.cpp`
- Test: `Source/PuertsRuntimePlugin/Private/Tests/PuertsRuntimeSubsystemTests.cpp`

- [ ] **Step 1: 写默认设置测试**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuertsRuntimeSettingsDefaultsTest,
    "PuertsRuntimePlugin.Settings.Defaults",
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
```

- [ ] **Step 2: 运行并确认测试失败**

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests PuertsRuntimePlugin.Settings;Quit" -TestExit="Automation Test Queue Empty" -log
```

Expected: FAIL，因为 `UPuertsRuntimeSettings` 尚不存在。

- [ ] **Step 3: 实现设置对象**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PuertsRuntimeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Puerts Runtime"))
class PUERTSRUNTIMEPLUGIN_API UPuertsRuntimeSettings final : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category="Startup") bool bAutoStart = true;
    UPROPERTY(Config, EditAnywhere, Category="Startup") FString EntryModule = TEXT("Main");
    UPROPERTY(Config, EditAnywhere, Category="Modules") FString ProjectScriptRoot = TEXT("JavaScript");
    UPROPERTY(Config, EditAnywhere, Category="Modules") FString PluginScriptRoot = TEXT("JavaScript");
    UPROPERTY(Config, EditAnywhere, Category="Debug", meta=(ClampMin="-1", ClampMax="65535")) int32 DebugPort = -1;

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
```

`.cpp` 内容如下：

```cpp
#include "PuertsRuntimeSettings.h"
```

再次执行 Step 2 命令，Expected: PASS。

- [ ] **Step 4: 提交设置**

```powershell
git add Source/PuertsRuntimePlugin/Public/PuertsRuntimeSettings.h Source/PuertsRuntimePlugin/Private/PuertsRuntimeSettings.cpp Source/PuertsRuntimePlugin/Private/Tests/PuertsRuntimeSubsystemTests.cpp
git commit -m "feat: add puerts runtime settings"
```

### Task 4: 实现每 GameInstance 的 gameplay VM

**Files:**
- Create: `Source/PuertsRuntimePlugin/Public/PuertsRuntimeGameInstanceSubsystem.h`
- Create: `Source/PuertsRuntimePlugin/Private/PuertsRuntimeHost.h`
- Create: `Source/PuertsRuntimePlugin/Private/PuertsRuntimeHost.cpp`
- Create: `Source/PuertsRuntimePlugin/Private/PuertsRuntimeLogger.h`
- Create: `Source/PuertsRuntimePlugin/Private/PuertsRuntimeLogger.cpp`
- Create: `Source/PuertsRuntimePlugin/Private/PuertsRuntimeGameInstanceSubsystem.cpp`
- Modify: `Source/PuertsRuntimePlugin/Public/PuertsRuntimePlugin.h`
- Modify: `Source/PuertsRuntimePlugin/Private/PuertsRuntimePlugin.cpp`
- Modify: `Source/PuertsRuntimePlugin/Private/Tests/PuertsRuntimeSubsystemTests.cpp`

- [ ] **Step 1: 定义可测试的 runtime host 并写生命周期幂等测试**

`PuertsRuntimeHost.h` 定义不依赖 UObject 的内部所有权边界：

```cpp
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
```

`Start` 在已有 environment 时直接返回 `true`；新 environment 的 `Start` 返回 false 时立即 reset。`Stop` 只 reset；`Restart` 先 stop 再 start。测试 fake 实现 `IPuertsRuntimeEnvironment`，通过引用计数器记录 Start 和析构次数：

```cpp
int32 CreateCount = 0;
int32 StartCount = 0;
int32 DestroyCount = 0;
FPuertsRuntimeHost Host([&]() {
    ++CreateCount;
    return MakeUnique<FFakePuertsRuntimeEnvironment>(StartCount, DestroyCount, true);
});
TestTrue(TEXT("first start succeeds"), Host.Start());
TestTrue(TEXT("second start is idempotent"), Host.Start());
TestEqual(TEXT("one environment"), CreateCount, 1);
TestEqual(TEXT("one environment start"), StartCount, 1);
TestTrue(TEXT("restart succeeds"), Host.Restart());
TestEqual(TEXT("restart creates a new environment"), CreateCount, 2);
TestEqual(TEXT("restart destroys the old environment"), DestroyCount, 1);
Host.Stop();
Host.Stop();
TestEqual(TEXT("final stop destroys once"), DestroyCount, 2);
```

- [ ] **Step 2: 运行并确认生命周期测试失败**

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests PuertsRuntimePlugin.Subsystem;Quit" -TestExit="Automation Test Queue Empty" -log
```

Expected: FAIL，因为 subsystem 与工厂 seam 尚不存在。

- [ ] **Step 3: 定义 subsystem 公共 API 和事件**

```cpp
DECLARE_MULTICAST_DELEGATE_OneParam(FPuertsRuntimeSourceLoaded, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPuertsRuntimeInstanceEvent, class UPuertsRuntimeGameInstanceSubsystem*);
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
    bool IsRunning() const { return RuntimeHost != nullptr && RuntimeHost->IsRunning(); }
    bool ReloadSource(const FString& AbsolutePath);
    const TSet<FString>& GetLoadedSourcePaths() const { return LoadedSourcePaths; }
    FPuertsRuntimeSourceLoaded& OnSourceLoaded() { return SourceLoadedEvent; }

private:
    TArray<FPuertsScriptRoot> BuildScriptRoots() const;
    TUniquePtr<FPuertsRuntimeHost> RuntimeHost;
    TSet<FString> LoadedSourcePaths;
    FPuertsRuntimeSourceLoaded SourceLoadedEvent;
};
```

`FPuertsRuntimePluginModule` 增加静态访问器 `OnRuntimeInstanceCreated()` 和 `OnRuntimeInstanceDestroyed()`；subsystem 在 `Initialize` 中先广播 created，再根据 `bAutoStart` 启动，在 `Deinitialize` 中先广播 destroyed，再停止 VM。

- [ ] **Step 4: 构建根目录并启动 FJsEnv**

先实现状态 logger；四个方法都转发给 `FDefaultLogger`，只有 `Error` 额外递增计数：

```cpp
class FPuertsRuntimeLogger final : public puerts::ILogger
{
public:
    virtual void Log(const FString& Message) const override { DefaultLogger.Log(Message); }
    virtual void Info(const FString& Message) const override { DefaultLogger.Info(Message); }
    virtual void Warn(const FString& Message) const override { DefaultLogger.Warn(Message); }
    virtual void Error(const FString& Message) const override
    {
        ++ErrorCount;
        DefaultLogger.Error(Message);
    }
    int32 GetErrorCount() const { return ErrorCount.Load(); }

private:
    puerts::FDefaultLogger DefaultLogger;
    mutable TAtomic<int32> ErrorCount{0};
};
```

```cpp
TArray<FPuertsScriptRoot> UPuertsRuntimeGameInstanceSubsystem::BuildScriptRoots() const
{
    const UPuertsRuntimeSettings* Settings = GetDefault<UPuertsRuntimeSettings>();
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("EasyEditorPlugin"));
    checkf(Plugin.IsValid(), TEXT("EasyEditorPlugin descriptor is not registered"));
    return {
        {TEXT("Project"), FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / Settings->ProjectScriptRoot)},
        {TEXT("EasyEditorPlugin"), FPaths::ConvertRelativePathToFull(Plugin->GetContentDir() / Settings->PluginScriptRoot)}
    };
}
```

`PuertsRuntimeHost.cpp` 中实现 `FPuertsJsEnvironment final : public IPuertsRuntimeEnvironment`，成员为 `TSharedPtr<puerts::FJsEnv> JsEnv` 和 `std::shared_ptr<FPuertsRuntimeLogger> Logger`。其构造函数接收 roots、settings、game instance 和 source-loaded callback；先用加载器搜索 `EntryModule`，找不到时记录 `GetLastSearchDiagnostic()` 并保持无效。找到后创建 `FJsEnv`，在 source-loaded callback 中规范化路径、加入 `LoadedSourcePaths` 并广播事件。`Start()` 在 `JsEnv` 无效时返回 false，否则执行：

```cpp
TArray<TPair<FString, UObject*>> Arguments;
Arguments.Emplace(TEXT("GameInstance"), GameInstance);
const int32 ErrorsBeforeStart = Logger->GetErrorCount();
JsEnv->Start(Settings->EntryModule, Arguments);
return Logger->GetErrorCount() == ErrorsBeforeStart;
```

host 收到 `Start() == false` 时立即 reset environment，确保入口同步异常不会留下“运行中”的 VM。为 host 增加 fake error 测试：environment 的 `Start` 返回 false 时，断言 host 返回 false、`IsRunning()` 为 false 且析构计数为 1。

`FPuertsJsEnvironment::ReloadSource` 使用 `FFileHelper::LoadFileToArray` 和 `puerts::PString`。subsystem 在 `Initialize` 中构建 `FPuertsRuntimeHost`，factory 捕获根目录、设置和 `TWeakObjectPtr<UPuertsRuntimeGameInstanceSubsystem>`；`StopRuntime` 调用 host 的 `Stop()` 并清空 `LoadedSourcePaths`。

- [ ] **Step 5: 运行 runtime 自动化测试**

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests PuertsRuntimePlugin;Quit" -TestExit="Automation Test Queue Empty" -log
```

Expected: Settings、Loader、Subsystem 测试全部 PASS。

- [ ] **Step 6: 提交 gameplay VM**

```powershell
git add Source/PuertsRuntimePlugin
git commit -m "feat: host puerts runtime per game instance"
```

### Task 5: 建立插件 TypeScript 可复用包

**Files:**
- Create: `TypeScript/package.json`
- Create: `TypeScript/tsconfig.json`
- Create: `TypeScript/scripts/write-runtime-package.mjs`
- Create: `TypeScript/src/runtime.ts`
- Create: `TypeScript/src/index.ts`
- Create: `TypeScript/src/EditorMain.ts`
- Create: `.gitignore`

- [ ] **Step 1: 定义独立构建配置**

```json
{
  "name": "@matrix/puerts-runtime-source",
  "version": "1.0.0",
  "private": true,
  "scripts": {
    "clean": "node -e \"require('fs').rmSync('../Content/JavaScript/node_modules/@matrix/puerts-runtime',{recursive:true,force:true});require('fs').rmSync('../Typing/@matrix/puerts-runtime',{recursive:true,force:true})\"",
    "build": "npm run clean && tsc -p tsconfig.json && node scripts/write-runtime-package.mjs",
    "typecheck": "tsc -p tsconfig.json --noEmit"
  },
  "devDependencies": {
    "typescript": "5.9.2"
  }
}
```

```json
{
  "compilerOptions": {
    "target": "ES2022",
    "module": "commonjs",
    "moduleResolution": "node",
    "strict": true,
    "skipLibCheck": true,
    "esModuleInterop": true,
    "sourceMap": true,
    "rootDir": "src",
    "outDir": "../Content/JavaScript/node_modules/@matrix/puerts-runtime",
    "declaration": true,
    "declarationDir": "../Typing/@matrix/puerts-runtime",
    "typeRoots": ["../../Puerts/Typing", "./node_modules/@types"]
  },
  "include": ["src/index.ts", "src/runtime.ts", "src/EditorMain.ts"]
}
```

- [ ] **Step 2: 实现最小可复用 gameplay 生命周期 API**

```ts
// TypeScript/src/runtime.ts
import * as UE from "ue";

export class GameplayRuntime {
    private started = false;

    public constructor(public readonly gameInstance: UE.GameInstance) {}

    public start(): void {
        if (this.started) return;
        this.started = true;
    }

    public stop(): void {
        if (!this.started) return;
        this.started = false;
    }
}
```

```ts
// TypeScript/src/index.ts
export { GameplayRuntime } from "./runtime";
```

`EditorMain.ts` 只注册插件自带编辑器功能，不导入 gameplay runtime；其入口可为空模块：`export {};`。

- [ ] **Step 3: 生成运行时 package.json**

```js
// TypeScript/scripts/write-runtime-package.mjs
import { mkdirSync, writeFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const output = resolve(scriptDir, "../../Content/JavaScript/node_modules/@matrix/puerts-runtime");
mkdirSync(output, { recursive: true });
writeFileSync(resolve(output, "package.json"), JSON.stringify({
  name: "@matrix/puerts-runtime",
  version: "1.0.0",
  main: "index.js"
}, null, 2) + "\n", "utf8");
```

- [ ] **Step 4: 构建并检查产物**

```powershell
Set-Location D:\MatrixTA\LyraRPGGameplayAbility\Plugins\EasyEditorPlugin\TypeScript
npm install
npm run typecheck
npm run build
Test-Path ..\Content\JavaScript\node_modules\@matrix\puerts-runtime\index.js
Test-Path ..\Typing\@matrix\puerts-runtime\index.d.ts
```

Expected: 两个 `Test-Path` 都输出 `True`，`npm run typecheck` 退出码为 0。

- [ ] **Step 5: 设置版本控制边界并提交**

`.gitignore` 增加以下内容；提交 `package-lock.json`、编译后的 runtime JavaScript 和声明文件，以便插件开箱即用且不要求消费项目安装 Node。

```gitignore
TypeScript/node_modules/
*.log
```

```powershell
git add .gitignore TypeScript Content/JavaScript Typing
git commit -m "feat: add reusable puerts gameplay package"
```

### Task 6: 将插件 JavaScript 纳入打包 staging

**Files:**
- Modify: `Source/PuertsRuntimePlugin/PuertsRuntimePlugin.Build.cs`
- Modify: `README.md`

- [ ] **Step 1: 声明 NonUFS runtime dependency**

在 runtime Build.cs 构造函数末尾增加：

```csharp
RuntimeDependencies.Add("$(PluginDir)/Content/JavaScript/...*.js", StagedFileType.NonUFS);
RuntimeDependencies.Add("$(PluginDir)/Content/JavaScript/...*.mjs", StagedFileType.NonUFS);
RuntimeDependencies.Add("$(PluginDir)/Content/JavaScript/...*.cjs", StagedFileType.NonUFS);
RuntimeDependencies.Add("$(PluginDir)/Content/JavaScript/...*.json", StagedFileType.NonUFS);
```

这会保留插件相对目录，使 `IPlugin::GetContentDir()` 在 staged build 中仍指向可读取的 loose files。

- [ ] **Step 2: 构建 Development staging 并验证文件**

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -project=D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -skiparchive
Get-ChildItem D:\MatrixTA\LyraRPGGameplayAbility\Saved\StagedBuilds -Recurse -Filter index.js | Where-Object FullName -Like '*EasyEditorPlugin*puerts-runtime*'
```

Expected: 恰好找到插件 `@matrix/puerts-runtime/index.js`。

- [ ] **Step 3: 记录 staging 约束并提交**

README 说明 JavaScript 使用 NonUFS staging，gameplay 包不得使用 Node.js API，Shipping 默认不依赖 `.map`。

```powershell
git add Source/PuertsRuntimePlugin/PuertsRuntimePlugin.Build.cs README.md
git commit -m "build: stage plugin javascript for packaged games"
```

### Task 7: 迁移原编辑器功能到 PuertsEditorPlugin

**Files:**
- Modify: `Source/PuertsEditorPlugin/Public/PuertsEditorPlugin.h`
- Modify: `Source/PuertsEditorPlugin/Private/PuertsEditorPlugin.cpp`
- Delete: `Source/EasyEditorPlugin/EasyEditorPlugin.Build.cs`
- Delete: `Source/EasyEditorPlugin/Public/EasyEditorPlugin.h`
- Delete: `Source/EasyEditorPlugin/Private/EasyEditorPlugin.cpp`

- [ ] **Step 1: 机械迁移现有绑定与模块状态**

移动旧 header/cpp 内容后执行以下重命名：

```text
FEasyEditorPluginModule -> FPuertsEditorPluginModule
IMPLEMENT_MODULE(..., EasyEditorPlugin) -> IMPLEMENT_MODULE(..., PuertsEditorPlugin)
LoadModuleChecked<...>("EasyEditorPlugin") -> LoadModuleChecked<...>("PuertsEditorPlugin")
EasyEditor.Restart -> PuertsEditor.Restart
```

保留 `struct EasyEditorPlugin` 作为 TypeScript 兼容 facade，避免现有 `cpp.EasyEditorPlugin` 调用失效；只改变它内部加载的模块名。依赖旧 C++ 模块 `EasyEditorPlugin` 的扩展必须把 Build.cs 依赖和 `LoadModuleChecked` 改为 `PuertsEditorPlugin`，这项源码迁移在 `Doc/runtime-gameplay.md` 明确列出。

- [ ] **Step 2: 使用共享加载器和 EditorMain**

编辑器 VM 的 `InitJsEnv` 改为创建项目根和插件根，并使用 `FPuertsMultiRootModuleLoader`。入口从 `Main` 改为 `@matrix/puerts-runtime/EditorMain`，启动前先 Search；缺少入口时输出一次 warning 并保持编辑器可用。第一阶段 watcher 只处理 TypeScript CommonJS 生成的 `.js`，不宣称支持 `.mjs/.cjs` 热重载。

```cpp
const TArray<FPuertsScriptRoot> Roots = BuildEditorScriptRoots();
ModuleLoader = std::make_shared<FPuertsMultiRootModuleLoader>(Roots);
JsEnv = MakeShared<puerts::FJsEnv>(ModuleLoader, std::make_shared<puerts::FDefaultLogger>(), -1,
    [this](const FString& Path) {
        SourceFileWatcher->OnSourceLoaded(FPuertsMultiRootModuleLoader::CanonicalizePath(Path));
    });
JsEnv->Start(TEXT("@matrix/puerts-runtime/EditorMain"));
```

实际传给 `FJsEnv` 的 loader/logger 使用 `std::shared_ptr`，不要混用 Unreal `TSharedPtr`；编辑器模块成员也采用与构造函数一致的所有权类型。

- [ ] **Step 3: 修复生命周期泄漏**

保存并在 `ShutdownModule` 中移除 `OnPostEngineInit`、`OnMapChanged` 和 ticker handles；注销 `UToolMenus` string command handler；清理 console commands 后再 reset VM。`Tick` 启动一次后返回 `false`，并将 ticker handle 清空。

- [ ] **Step 4: 编译并运行编辑器 smoke test**

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Build\BatchFiles\Build.bat LyraEditor Win64 Development D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -WaitMutex
D:\UnrealEngine\UnrealEngine5\Engine\Binaries\Win64\UnrealEditor.exe D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -ExecCmds="PuertsEditor.Restart"
```

Expected: 编译成功；日志出现 `EditorMain` 启动信息；退出时没有 stale delegate 或 console command 断言。

- [ ] **Step 5: 提交编辑器迁移**

```powershell
git add EasyEditorPlugin.uplugin Source
git commit -m "refactor: migrate easy editor features to puerts editor module"
```

### Task 8: 为 gameplay VM 添加编辑器热重载和重启

**Files:**
- Create: `Source/PuertsEditorPlugin/Private/PuertsRuntimeHotReloadManager.h`
- Create: `Source/PuertsEditorPlugin/Private/PuertsRuntimeHotReloadManager.cpp`
- Modify: `Source/PuertsEditorPlugin/Public/PuertsEditorPlugin.h`
- Modify: `Source/PuertsEditorPlugin/Private/PuertsEditorPlugin.cpp`

- [ ] **Step 1: 定义 runtime watcher 所有权**

```cpp
class FPuertsRuntimeHotReloadManager
{
public:
    void Startup();
    void Shutdown();

private:
    void Attach(UPuertsRuntimeGameInstanceSubsystem* Runtime);
    void Detach(UPuertsRuntimeGameInstanceSubsystem* Runtime);
    TMap<TWeakObjectPtr<UPuertsRuntimeGameInstanceSubsystem>, TSharedPtr<puerts::FSourceFileWatcher>> Watchers;
    FDelegateHandle CreatedHandle;
    FDelegateHandle DestroyedHandle;
};
```

- [ ] **Step 2: 连接 source-loaded 与 ReloadSource**

`Attach` 创建 watcher，回调捕获 `TWeakObjectPtr`；路径变化时先调用 `CanonicalizePath`，再仅在 game thread 且 subsystem 有效时调用 `ReloadSource`。创建 watcher 后，把 `GetLoadedSourcePaths()` 中已有路径规范化后逐个传给 `OnSourceLoaded`，避免晚连接漏掉入口依赖。第一阶段只验证 CommonJS `.js` 热重载。

- [ ] **Step 3: 注册 gameplay 重启命令**

`PuertsRuntime.Restart` 遍历 `GEngine->GetWorldContexts()`，只处理 `PIE` 或 `Game` world 的 `UGameInstance`，对每个唯一 subsystem 调用一次 `RestartRuntime()`。没有活动 game instance 时输出 warning，不创建编辑器假 VM。

- [ ] **Step 4: 手工验证双客户端隔离**

在 PIE 设置 `Number of Players = 2`，运行后执行：

```text
PuertsRuntime.Restart
```

Expected: 日志显示两个不同 game instance 各重启一次；修改插件 `runtime.js` 后两个 VM 各 reload 一次；停止 PIE 后 watcher map 为空。

- [ ] **Step 5: 提交热重载支持**

```powershell
git add Source/PuertsEditorPlugin
git commit -m "feat: add editor hot reload for gameplay runtimes"
```

### Task 9: 让 Lyra 项目只保留调用代码

**Files:**
- Modify: `D:/MatrixTA/LyraRPGGameplayAbility/LyraStarterGame.uproject`
- Create: `D:/MatrixTA/LyraRPGGameplayAbility/TypeScript/package.json`
- Create: `D:/MatrixTA/LyraRPGGameplayAbility/TypeScript/package-lock.json` (generated by `npm install`)
- Create: `D:/MatrixTA/LyraRPGGameplayAbility/TypeScript/tsconfig.json`
- Create: `D:/MatrixTA/LyraRPGGameplayAbility/TypeScript/Main.ts`

- [ ] **Step 1: 确认验证项目启用插件**

复用“执行前准备”创建的 junction，并确认 `.uproject` 中存在以下启用项：

在 `.uproject` 的 `Plugins` 数组添加：

```json
{
  "Name": "EasyEditorPlugin",
  "Enabled": true
}
```

- [ ] **Step 2: 创建仅包含入口构建的项目 tsconfig**

`TypeScript/package.json`：

```json
{
  "name": "lyra-puerts-entry",
  "version": "1.0.0",
  "private": true,
  "scripts": {
    "build": "tsc -p tsconfig.json",
    "typecheck": "tsc -p tsconfig.json --noEmit"
  },
  "devDependencies": {
    "typescript": "5.9.2"
  }
}
```

`TypeScript/tsconfig.json`：

```json
{
  "compilerOptions": {
    "target": "ES2022",
    "module": "commonjs",
    "moduleResolution": "node",
    "strict": true,
    "skipLibCheck": true,
    "sourceMap": true,
    "outDir": "../Content/JavaScript",
    "baseUrl": ".",
    "paths": {
      "@matrix/puerts-runtime": ["../Plugins/EasyEditorPlugin/Typing/@matrix/puerts-runtime/index"]
    },
    "typeRoots": ["../Plugins/Puerts/Typing", "./node_modules/@types"]
  },
  "include": ["Main.ts"]
}
```

- [ ] **Step 3: 编写项目组合入口**

```ts
import * as UE from "ue";
import * as puerts from "puerts";
import { GameplayRuntime } from "@matrix/puerts-runtime";

const gameInstance = puerts.argv.getByName("GameInstance") as UE.GameInstance;
const runtime = new GameplayRuntime(gameInstance);
runtime.start();
```

项目 `TypeScript` 下不新增 gameplay 实现文件。

- [ ] **Step 4: 编译项目入口并运行 PIE**

```powershell
Set-Location D:\MatrixTA\LyraRPGGameplayAbility\TypeScript
npm install
npm run typecheck
npm run build
```

Expected: 只生成 `Content/JavaScript/Main.js` 与 `Main.js.map`；没有生成或复制 `@matrix/puerts-runtime`。

运行 PIE，Expected: `Main.js` 从项目 Content 加载，`@matrix/puerts-runtime` 从插件 Content 加载，日志无 `can not find`。

- [ ] **Step 5: 分别提交两个仓库的变更**

插件仓库只提交文档补充；Lyra 仓库提交 `.uproject`、三个 TypeScript 文件和 `package-lock.json`，不提交 junction 与生成的 `node_modules`。

### Task 10: 完成自动化、打包与迁移文档验收

**Files:**
- Modify: `README.md`
- Create: `Doc/runtime-gameplay.md`
- Modify: `docs/superpowers/specs/2026-07-10-puerts-runtime-plugin-design.md` only if implementation discovered a verified constraint

- [ ] **Step 1: 运行全部插件自动化测试**

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests PuertsRuntimePlugin;Quit" -TestExit="Automation Test Queue Empty" -log
```

Expected: Loader、Settings、Subsystem 全部 `Success`，进程退出码 0。

- [ ] **Step 2: 验证 Development 和 Shipping 包**

先分别编译不含 Editor 模块的 Game、Client、Server target：

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Build\BatchFiles\Build.bat LyraGame Win64 Development D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -WaitMutex
D:\UnrealEngine\UnrealEngine5\Engine\Build\BatchFiles\Build.bat LyraClient Win64 Development D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -WaitMutex
D:\UnrealEngine\UnrealEngine5\Engine\Build\BatchFiles\Build.bat LyraServer Win64 Development D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -WaitMutex
```

Expected: 三个 target 均 `Result: Succeeded`，链接输入不包含 `PuertsEditorPlugin`。

再执行打包：

```powershell
D:\UnrealEngine\UnrealEngine5\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -project=D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory=D:\MatrixTA\Artifacts\LyraPuertsDevelopment
D:\UnrealEngine\UnrealEngine5\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -project=D:\MatrixTA\LyraRPGGameplayAbility\LyraStarterGame.uproject -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory=D:\MatrixTA\Artifacts\LyraPuertsShipping
```

Expected: 两次均 `AutomationTool exiting with ExitCode=0`。启动两个包，确认项目 `Main` 和插件 `@matrix/puerts-runtime` 均被加载，且 Shipping 日志中没有模块缺失。

- [ ] **Step 3: 文档化消费项目步骤**

`Doc/runtime-gameplay.md` 必须明确：安装 Puerts 与 EasyEditorPlugin、构建插件 TS 包、项目 `paths` 配置、项目 `Main.ts` 示例、裸模块导入规则、项目覆盖优先级、PIE 多实例语义、NonUFS staging 和不支持 Node API 的约束。

- [ ] **Step 4: 检查 runtime/editor 依赖边界**

```powershell
rg -n "UnrealEd|ToolMenus|Slate|ContentBrowser|LevelEditor" Source/PuertsRuntimePlugin
rg -n "EasyEditorScripts|LoadModuleChecked.*EasyEditorPlugin" Source
git diff --check
```

Expected: 第一条和第二条均无输出；`git diff --check` 无输出。

- [ ] **Step 5: 最终提交**

```powershell
git add README.md Doc/runtime-gameplay.md docs/superpowers/specs/2026-07-10-puerts-runtime-plugin-design.md
git commit -m "docs: document reusable puerts gameplay workflow"
```

## 完成标准

- `PuertsRuntimePlugin` 可被 Game/Client/Server target 编译，且不依赖任何 Editor 模块。
- 每个 `UGameInstance` 恰好拥有一个 gameplay VM，销毁 game instance 时 VM 同步销毁。
- 项目 `Main.js` 优先从项目 Content 读取，`@matrix/puerts-runtime` 从插件 Content 读取。
- 项目 TypeScript 目录只有入口、配置和依赖元数据，没有插件 gameplay 实现副本。
- PIE 多客户端、Standalone、Development 与 Shipping 包均通过。
- 原 `cpp.EasyEditorPlugin` 编辑器 API 保持兼容，所有 delegate、ticker、watcher 和 console command 均可清理。
