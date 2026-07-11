# Puerts Gameplay Runtime

## 安装

项目需要同时安装腾讯 Puerts 和 EasyEditorPlugin，并在 `.uproject` 中启用 `EasyEditorPlugin`。插件描述符会启用 Puerts。Unreal 模块职责如下：

- `PuertsRuntimePlugin`：Game、Client、Server 可用；每个 `UGameInstance` 管理一个 gameplay VM。
- `PuertsEditorPlugin`：仅编辑器可用；保留原编辑器绑定、编辑器 VM、文件 watcher 和重启命令。

Puerts 的 UE 类型声明需要针对消费项目生成。完整声明应位于项目可访问的 type root，例如 `Typing/ue`；Puerts 仓库自带的占位声明不能替代项目生成的 `ue.d.ts` 和 `ue_bp.d.ts`。

## 构建插件包

可复用 TypeScript 源码位于插件 `TypeScript/src`。在插件目录执行：

```powershell
cd Plugins/EasyEditorPlugin/TypeScript
npm install
npm run typecheck
npm run build
```

构建产物是 `Content/JavaScript/node_modules/@matrix/puerts-runtime`，声明文件位于 `Typing/@matrix/puerts-runtime`。消费项目不复制这些实现文件。

## 项目入口

项目 `TypeScript` 目录只保存入口、`tsconfig.json` 和 npm 元数据。`tsconfig.json` 至少包含以下映射：

```json
{
  "compilerOptions": {
    "module": "commonjs",
    "moduleResolution": "node",
    "outDir": "../Content/JavaScript",
    "paths": {
      "@matrix/puerts-runtime": ["../Plugins/EasyEditorPlugin/Typing/@matrix/puerts-runtime/index"]
    },
    "typeRoots": ["./node_modules/@types", "../Plugins/Puerts/Typing"]
  }
}
```

项目入口示例：

```typescript
import * as UE from "ue";
import * as puerts from "puerts";
import { GameplayRuntime } from "@matrix/puerts-runtime";

const gameInstance = puerts.argv.getByName("GameInstance") as UE.GameInstance;
const runtime = new GameplayRuntime(gameInstance);
runtime.start();
```

构建后项目只需要生成 `Content/JavaScript/Main.js`；插件包仍留在插件 Content 中。

## 模块解析

加载根顺序固定为项目 `Content/JavaScript`、插件 `Content/JavaScript`。因此项目可以覆盖同名顶层模块。相对导入只在发起导入的目录中解析，不跨根回退；裸模块导入按 `node_modules` 向上查找，然后按根顺序查找。

Puerts 读取 `package.json` 后会把 `main` 值作为相对包目录的请求再次交给 loader，即使该值没有 `./` 前缀。loader 已兼容常见的 `"main": "index.js"` 和 `"main": "dist/main.js"`。

## 生命周期与热重载

每个 `UGameInstance` 拥有独立 VM。PIE 多客户端不会共享全局脚本状态；GameInstance 销毁时对应 VM 同步销毁。脚本可通过名为 `GameInstance` 的 Puerts argv 参数取得当前实例。

编辑器第一阶段只对已加载的 CommonJS `.js` 文件执行热重载。控制台命令：

- `PuertsRuntime.Restart`：重启当前 PIE/Game world 的全部 gameplay VM。
- `PuertsEditor.Restart`：重启独立的编辑器工具 VM。

`.mjs`、`.cjs` 可以参与模块加载，但当前 watcher 不承诺对它们热重载。

## 打包约束

插件包、项目 `Content/JavaScript` 根目录入口以及 Puerts 必需的 `puerts/*.js` bootstrap 由 `PuertsRuntimePlugin.Build.cs` 作为 NonUFS loose files staging。项目的编辑器/开发子目录不会递归进入 Shipping。项目入口与插件包都必须保留原相对目录；`.map` 不进入 staging，Shipping 不依赖 source map。

UBT 会把项目 `Content/JavaScript` 根目录已有的 `.js`、`.mjs`、`.cjs`、`.json` 作为入口文件逐个 staging，因此 `UPuertsRuntimeSettings.EntryModule` 可以使用其他顶层入口名。TypeScript 必须在 Unreal target 构建前完成编译，确保入口文件在 UBT 生成 receipt 时已经存在。

Gameplay 代码运行在 Puerts V8 环境中，不得依赖 Node.js 内置模块、文件系统 API 或 npm 安装时环境。需要原生能力时应通过 Unreal/Puerts 绑定显式提供。

## 旧编辑器模块迁移

TypeScript 的 `cpp.EasyEditorPlugin` facade 保持兼容。依赖旧 C++ 模块的扩展需要把 Build.cs 依赖和 `LoadModuleChecked` 名称从 `EasyEditorPlugin` 改为 `PuertsEditorPlugin`。
