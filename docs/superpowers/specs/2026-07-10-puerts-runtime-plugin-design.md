# Puerts Runtime Plugin Design

## 目标

把 EasyEditorPlugin 从仅编辑器可用的单一 Puerts 环境，拆分为可打包的 gameplay runtime 模块和编辑器辅助模块。可复用 TypeScript 源码及其编译产物归插件所有；项目 `TypeScript` 目录只保留入口、配置和调用代码。

物理插件名称继续使用 `EasyEditorPlugin`，避免破坏已有项目中的插件启用项和安装路径。Unreal 模块名称调整为：

- `PuertsRuntimePlugin`：`Runtime` 模块，负责 gameplay VM、模块加载、设置和生命周期。
- `PuertsEditorPlugin`：`Editor` 模块，负责现有编辑器绑定、编辑器脚本环境、热重载和控制台命令。

## 目录与所有权

```text
EasyEditorPlugin/
  EasyEditorPlugin.uplugin
  Source/
    PuertsRuntimePlugin/
    PuertsEditorPlugin/
  TypeScript/
    src/
    package.json
    tsconfig.json
  Content/
    JavaScript/
      node_modules/
        @matrix/
          puerts-runtime/
  Typing/
    @matrix/
      puerts-runtime/
```

插件的 `TypeScript/src` 是可复用 gameplay 代码的唯一源码位置。它被独立编译为包名 `@matrix/puerts-runtime`；JavaScript 输出到插件 `Content/JavaScript/node_modules/@matrix/puerts-runtime`，声明文件输出到插件 `Typing/@matrix/puerts-runtime`。

项目目录保持精简：

```text
Project/
  TypeScript/
    Main.ts
    package.json
    tsconfig.json
  Content/
    JavaScript/
      Main.js
```

项目通过 TypeScript `paths` 读取插件声明，通过运行时裸模块导入读取插件 JavaScript：

```ts
import { installGameplay } from "@matrix/puerts-runtime";

installGameplay(GameInstance);
```

## Runtime 架构

`UPuertsRuntimeGameInstanceSubsystem` 为每个 `UGameInstance` 创建一个独立 `FJsEnv`。这保证 PIE 多客户端、Standalone 和打包游戏之间的 VM 生命周期相互隔离。默认入口是项目 `Content/JavaScript/Main.js`，并把当前 `UGameInstance` 作为启动参数传给入口模块。

`FPuertsMultiRootModuleLoader` 实现 `puerts::IJSModuleLoader`，严格复刻 Puerts 默认候选顺序：显式 `.js`、`.mjs`、`.cjs`、`.json`，无扩展名时依次尝试 `.js`、`.mjs`、`.cjs`、条件编译的 `.mbc/.cbc`、`package.json` 和 `index.js`。`package.json` 原样返回给 Puerts 解析 `main`。根目录固定按以下顺序搜索：

1. 项目 `Content/JavaScript`。
2. EasyEditorPlugin 的 `Content/JavaScript`。

裸模块先遵循 Node 语义，从发起文件目录向上查找最近的 `node_modules`；仍未命中时才按“项目根、插件根”回退，因此项目入口可覆盖插件包，而插件包的内部依赖保持就近解析。`./`、`../` 相对导入只从当前已加载文件所在目录解析，失败后不查 `node_modules`、不跨根回退；绝对导入只检查自身；`@scope/package` 及其子路径属于裸模块。加载器把 `Path` 和 `AbsolutePath` 都设为规范化绝对正斜杠路径，使模块缓存、热重载、错误日志和 packaged loose files 使用同一个模块标识。

`UPuertsRuntimeSettings` 提供以下配置并写入项目配置：

- `bAutoStart`，默认 `true`。
- `EntryModule`，默认 `Main`。
- `ProjectScriptRoot`，默认 `JavaScript`。
- `PluginScriptRoot`，默认 `JavaScript`。
- `DebugPort`，默认 `-1`。

启动使用一个转发到 `FDefaultLogger` 的状态 logger。它记录 `FJsEnv::Start` 同步阶段新增的 error 数量；入口缺失或启动阶段产生 JavaScript error 时，runtime host 销毁该 VM 并返回失败。错误必须记录入口模块、请求模块、发起目录和全部搜索根。运行时不静默回退到另一个 VM，也不吞掉 JavaScript 异常。

## Editor 架构

`PuertsEditorPlugin` 接管原 `EasyEditorPlugin` 模块中的 Slate、ToolMenus、Content Browser 和控制台绑定，继续维持独立的编辑器工具 VM。编辑器入口改为插件包子模块 `@matrix/puerts-runtime/EditorMain`，避免与 gameplay 的项目入口 `Main` 混用。第一阶段热重载只承诺 TypeScript CommonJS 输出的 `.js` 文件；`.mjs`、`.cjs` 与字节码仍可加载，但不承诺 watcher reload。

编辑器模块复用 `FPuertsMultiRootModuleLoader`，监听其实际加载的绝对文件路径。文件变化时，仅对拥有该文件的 VM 调用 `ReloadSource`。`PuertsRuntime.Restart` 重启当前 PIE/Standalone game instance 的 gameplay VM；`PuertsEditor.Restart` 重启编辑器工具 VM。所有 delegate、ticker、console command 和 watcher handle 在关闭模块或 VM 时成对注销。

## 构建与打包

插件 TypeScript 包独立执行 `npm run build`，项目 TypeScript 构建先构建插件包，再只编译项目入口。项目源码不复制插件实现。

插件 `Build.cs` 通过 `RuntimeDependencies` 将 `Content/JavaScript` 下的运行时文件加入 staging。TypeScript 源码、source map 和声明文件默认不进入 Shipping 包；Development 构建可选择保留 source map。目标平台必须启用 Puerts 所需 V8 后端；可复用 gameplay 包不得依赖 Node.js API，从而避免 `WITH_NODEJS` 和平台差异。

## 测试与验收

自动化测试覆盖：

- 项目根优先于插件根。
- 项目缺失时回退到插件包。
- 插件包内相对导入。
- 相对导入缺失时不跨根目录回退。
- 裸模块的向上 `node_modules` 查找。
- 显式 JSON、`package.json main`、`index.js` 和条件字节码扩展解析。
- `Path` 与 `AbsolutePath` 使用完全相同的规范化绝对路径。
- 入口同步抛错时 runtime host 返回失败并销毁 VM。
- 缺失模块错误包含请求上下文和搜索根。
- subsystem 的重复启动、停止和销毁是幂等的。

集成验收覆盖 Editor PIE 单客户端、PIE 多客户端、Standalone、Development 打包和 Shipping 打包。通过标准是项目 `TypeScript/Main.ts` 只有组合调用，且打包后能够从插件目录加载 `@matrix/puerts-runtime`，不依赖项目中复制的插件 JavaScript。

## 非目标

- 不修改 Puerts 上游源码。
- 不在第一阶段实现运行时代码热更新下载、签名或版本协商。
- 不允许 gameplay 包依赖 `UnrealEd`、Slate、ToolMenus 或 Node.js API。
- 不把插件 TypeScript 源码复制到项目的受版本控制源码目录。
