# 插件 Runtime TypeScript

项目和插件共用项目根目录的一个 `tsconfig.json`。插件保存 TS 源码，Puerts 在编辑器内监听并编译到项目 `Content/JavaScript`。游戏继续使用已有的 Runtime JsEnv。

本插件提供 `TypeScript/Runtime/index.ts` 和 `Greeting.ts` 作为最小样例，不自动执行业务逻辑，也不依赖编辑器 API。

## 接入

1. 将 EasyEditorPlugin 放入项目 `Plugins/EasyEditorPlugin`。Windows 开发时可以创建链接：

   ```powershell
   New-Item -ItemType SymbolicLink -Path 'D:/YourGame/Plugins/EasyEditorPlugin' -Target 'D:/MatrixTA/EasyEditorPlugin'
   ```

2. 按下面的完整示例配置项目根 `tsconfig.json`。这是参考配置，不是插件自己的独立配置；接入已有项目时合并必要选项，不要直接覆盖原有配置。

   ```json
   {
     "compilerOptions": {
       "target": "ESNext",
       "module": "CommonJS",
       "moduleResolution": "node",
       "experimentalDecorators": true,
       "useDefineForClassFields": false,
       "sourceMap": true,
       "rootDir": ".",
       "outDir": "Content/JavaScript",
       "typeRoots": ["Typing", "node_modules/@types"],
       "skipLibCheck": true
     },
     "include": [
       "TypeScript/**/*.ts",
       "TypeScript/**/*.tsx",
       "Plugins/EasyEditorPlugin/TypeScript/Runtime/**/*.ts"
     ],
     "exclude": ["**/node_modules/**", "Content/JavaScript/**"]
   }
   ```

   其他插件按相同方式逐个添加 include。只纳入 Runtime 源码，避免将编辑器脚本、测试及禁用插件一起编译。

3. 在项目 `Config/DefaultPuerts.ini` 中启用原有编译监听：

   ```ini
   [/Script/Puerts.PuertsSetting]
   AutoModeEnable=True
   WatchDisable=False
   ```

   Puerts 的编辑器编译器依赖 AutoModeEnable，即使游戏自行创建 JsEnv 也需要开启。该开关同时启用 Puerts 默认环境，但不会自动执行你的游戏 Main；游戏入口仍由宿主管理。

4. 确认项目 `Content/JavaScript/PuertsEditor` 已有 Puerts 的编译器脚本和 `node_modules/typescript`。初始化 Puerts 后可执行：

   ```powershell
   npm install --prefix Content/JavaScript/PuertsEditor --ignore-scripts --no-audit --no-fund
   ```

   本次验证使用该目录 package.json 锁定的 TypeScript 4.7.4；项目原有 CLI 4.9.5 也通过编译。不要同时运行外部 `tsc --watch` 和 UE 内 Watch，避免两个写入者。

5. 项目入口直接引用插件模块：

   ```ts
   // 项目 TypeScript/Main.ts
   import { pluginGreeting } from "../Plugins/EasyEditorPlugin/TypeScript/Runtime";
   console.log(pluginGreeting("MyGame"));
   ```

6. Runtime 使用项目 `Content/JavaScript` 作为脚本根，启动 `TypeScript/Main`。原先 `Start("Main")` 的调用点需要改为 `Start("TypeScript/Main")`。

## 链接插件需要的 Puerts 小补丁

Windows 下只监听项目根目录，收不到外部链接目标中的源码修改。本次在 Puerts `PuertsEditor/CodeAnalyze.ts` 中增加了对 TypeScript 解析所得 include 根目录的监听；所有监听仍调用同一个编译服务，保留既有 MD5 去重逻辑。

需要一起交付 Puerts 仓库以下文件：

- `unreal/Puerts/PuertsEditor/CodeAnalyze.ts`
- `unreal/Puerts/Content/JavaScript/PuertsEditor/CodeAnalyze.js`
- `unreal/Puerts/Content/JavaScript/PuertsEditor/CodeAnalyze.js.map`

项目实际运行的是 `Content/JavaScript/PuertsEditor` 下的副本。关闭编辑器后将补丁中的 JS 及 map 复制到该目录，再重新启动编辑器。只更新插件里的 TS 源码不会更新已经复制到项目中的编译器。

链接插件必须在 include 中列出具体目录，例如 `Plugins/EasyEditorPlugin/TypeScript/Runtime/**/*.ts`。不要用一个宽泛的 `Plugins/**/*.ts` 代替：其监听根会落在 Plugins，仍可能跨不过链接边界。`files` 精确列表、任意项目外绝对路径和运行中新增插件目录不属于此补丁的自动发现范围。修改配置、启用新插件或重新挂载链接后重启编辑器。

补丁没有改变输出映射，也没有增加 tsconfig 私有字段。当前生成 JS 与 TS 源码保持一致，同时恢复了原先生成文件缺失的编译诊断输出。

## 输出与迁移

```text
TypeScript/Main.ts
  → Content/JavaScript/TypeScript/Main.js
Plugins/EasyEditorPlugin/TypeScript/Runtime/index.ts
  → Content/JavaScript/Plugins/EasyEditorPlugin/TypeScript/Runtime/index.js
```

目录层级保持不变，跨项目/插件的相对 import 编译后仍然成立。首版不引入 `@Plugin` 别名；tsconfig 的 paths 不会自动重写生成的 require。

如果旧配置在 `TypeScript/tsconfig.json`：

- 把配置迁移到项目根，删除旧配置，重新计算 outDir、rootDir、typeRoots、include、exclude 等相对路径。
- 如需保留 `baseUrl` 的原有含义，可将其设为 `TypeScript`，保留相对于该 baseUrl 的 paths。
- 若 package.json 仍在 TypeScript 目录，将命令改为 `tsc -p ../tsconfig.json` 与 `tsc -p ../tsconfig.json --watch`。
- 调整 Runtime 入口，并核对 TS 蓝图的模块路径。已有 TS 蓝图迁移需要单独验收，本次没有迁移业务蓝图资产。
- 保留第三方运行时依赖，例如项目已有的 `Content/JavaScript/node_modules` 和 react-umg；不要把整个 JavaScript 目录当作可删除的缓存。

## EasyEditor 编辑器脚本

基线编辑器环境仍启动 `Content/EasyEditorScripts/Main.js`，游戏脚本由游戏 Runtime 单独启动。插件 Runtime TS 不会自动成为编辑器入口。

如果宿主尚无编辑器入口，可添加以下最小 JS，然后逐步替换成实际工具代码：

```js
const editor = require("cpp").EasyEditorPlugin;
editor.SetOnJsEnvPreReload(() => {
    // 在此释放编辑器工具注册的回调、菜单等。
});
```

`EasyEditor.Restart` 使用该回调完成旧环境的退出，再启动编辑器入口。不要让其回退到旧的游戏 Main.js。

## 删除、重命名和错误恢复

保存有效 TS 会自动更新输出。类型或语法错误会记录到 UE Output Log；出错文件的旧 JS 可能继续存在，因此看到旧行为不代表编译成功。

Puerts 现有监听不自动清理被删除源码的历史输出。删除或重命名后：

1. 更新 import，确认新模块编译成功。
2. 关闭相关 Runtime 和编辑器。
3. 只删除该旧模块对应的 `.js`、`.js.map`、`.d.ts`、`.d.ts.map`，不要递归清空输出根目录。
4. 重新启动编辑器以刷新编译器文件列表和版本缓存，验证旧模块不再可加载。

旧入口迁移后也按此方式处理，但已有用户产物应在核对引用后再清理。本次 Lyra 集成没有删除其原有 JS/UI 文件。

## 验收范围

自动编译、下一次 Runtime 启动加载和运行中热更新是不同层次。Lyra 自建的 JsEnv 没有在本次新增热重载接线；测试以重新开始 PIE 加载最新代码为准。已有 WaitDebugger 设置保留，开始 PIE 时应连接现有调试端口并放行。

实际验证记录见 `Doc/Tests/2026-10-03-plugin-typescript.md`。打包、独立插件分发及游戏视觉表现不在本次验收范围。
