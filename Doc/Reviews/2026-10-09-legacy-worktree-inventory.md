# 旧 Runtime 工作树盘点

日期：2026-10-09。盘点后经用户授权，已移除旧工作树；下文记录清理前的文件清单。

## 身份与建议

- 路径：D:/MatrixTA/EasyEditorPlugin/.worktrees/puerts-runtime-plugin
- 注册的 Git 工作树，分支 codex/puerts-runtime-plugin，HEAD 41dbfd9。
- 共 40 个已跟踪文件；相对 889db468 有 21 个分支提交，代码可通过旧分支找回。
- 它是一套旧实现，不是单纯的缓存。当前最简方案不需要整体迁入。
- 已按用户后续指示移除工作树，保留 `codex/puerts-runtime-plugin` 分支；未将旧框架迁入当前实现。

## 可清理的生成产物

| 路径 | 文件数 | 约 MiB | 判断 |
| --- | ---: | ---: | --- |
| Binaries/ | 7 | 99.5 | 旧模块 DLL/PDB 等，可重新构建，不应迁入新方案 |
| Intermediate/ | 83 | 49.5 | UBT/UHT 中间产物，可重新生成 |
| Developer/TypeScript/node_modules/ | 139 | 22.5 | npm 依赖，可按旧锁文件重新安装 |

合计约 171.5 MiB，已随工作树清理。使用 `git worktree remove --force` 移除注册，随后仅移除残留 junction 本身和空目录，没有删除 junction 指向的外部 Typing 目录。旧分支保留已提交源码。

## 未提交内容

- TypeScript/index.ts：增加一条 GameplayRuntime.start!!! 日志，文件末尾无换行。
- TypeScript/runtime.ts：start/stop 各增加一条日志。

这些修改没有进入 41dbfd9，已保存到本仓库本地 `.git/worktree-backups/2026-10-09-puerts-runtime-plugin/`，包括 `uncommitted.patch`、两个 TS 文件原件及 HEAD/状态信息。移除前通过反向应用检查验证补丁与工作树一致。备份不纳入 Git 提交；恢复旧分支工作树后可用 `git apply` 应用补丁。

## 哪些有参考价值

- PuertsRuntimeHotReloadManager.cpp/.h 与其测试：可参考监听挂接、重置、卸载以及运行中刷新。依赖旧 Subsystem 与事件接口，不能直接复制后使用。
- PuertsRuntimeHost.cpp/.h、PuertsRuntimeGameInstanceSubsystem.cpp/.h 与测试：可参考每个 GameInstance 管理 JsEnv 生命周期；当前项目已经自行管理环境，迁入会引入重复职责。
- PuertsRuntimeSettings.cpp/.h：只有未来需要统一 Runtime 配置时才有价值。
- PuertsRuntimePlugin.Build.cs：打包 RuntimeDependencies 的参考，但旧规则针对 @matrix 包和项目 JS 顶层；不能直接覆盖当前嵌套输出布局。
- PuertsRuntimePlugin.cpp：包含旧的多配置注册和 tsconfig 增补，违背当前显式单配置方向，不建议迁入。
- Developer/TypeScript 下多个 tsconfig 与 Typing/@matrix 声明：服务于旧包结构，不适合当前布局。
- Doc/runtime-gameplay.md 与 docs/superpowers 两份文档：可保留旧分支作设计历史，不作为新实现使用说明。

## 全部已跟踪文件

### Runtime 生命周期与设置（仅未来扩展参考）

- `Source/PuertsRuntimePlugin/Private/PuertsRuntimeGameInstanceSubsystem.cpp`
- `Source/PuertsRuntimePlugin/Private/PuertsRuntimeHost.cpp`
- `Source/PuertsRuntimePlugin/Private/PuertsRuntimeHost.h`
- `Source/PuertsRuntimePlugin/Private/PuertsRuntimePlugin.cpp`
- `Source/PuertsRuntimePlugin/Private/PuertsRuntimeSettings.cpp`
- `Source/PuertsRuntimePlugin/Private/Tests/PuertsRuntimeSubsystemTests.cpp`
- `Source/PuertsRuntimePlugin/Public/PuertsRuntimeGameInstanceSubsystem.h`
- `Source/PuertsRuntimePlugin/Public/PuertsRuntimePlugin.h`
- `Source/PuertsRuntimePlugin/Public/PuertsRuntimeSettings.h`
- `Source/PuertsRuntimePlugin/PuertsRuntimePlugin.Build.cs`

### 编辑器与 Runtime 热重载（仅未来扩展参考）

- `Source/PuertsEditorPlugin/Private/PuertsEditorEnvironment.h`
- `Source/PuertsEditorPlugin/Private/PuertsEditorPlugin.cpp`
- `Source/PuertsEditorPlugin/Private/PuertsRuntimeHotReloadManager.cpp`
- `Source/PuertsEditorPlugin/Private/PuertsRuntimeHotReloadManager.h`
- `Source/PuertsEditorPlugin/Private/Tests/PuertsEditorPluginTests.cpp`
- `Source/PuertsEditorPlugin/Private/Tests/PuertsRuntimeHotReloadManagerTests.cpp`
- `Source/PuertsEditorPlugin/Public/PuertsEditorPlugin.h`
- `Source/PuertsEditorPlugin/PuertsEditorPlugin.Build.cs`

### 旧版 TS 样例与开发配置（当前不迁入）

- `Developer/TypeScript/editor/EditorMain.ts`
- `Developer/TypeScript/package-lock.json`
- `Developer/TypeScript/package.json`
- `Developer/TypeScript/tests/runtime.contract.ts`
- `Developer/TypeScript/tsconfig.editor.json`
- `Developer/TypeScript/tsconfig.json`
- `Developer/TypeScript/tsconfig.test.json`
- `TypeScript/index.ts`
- `TypeScript/runtime.ts`
- `Typing/@matrix/puerts-runtime/EditorMain.d.ts`
- `Typing/@matrix/puerts-runtime/index.d.ts`
- `Typing/@matrix/puerts-runtime/runtime.d.ts`

### 旧版说明与工程文件（历史保留）

- `.gitignore`
- `Doc/Pic/easyeditor.gif`
- `Doc/extension.md`
- `Doc/runtime-gameplay.md`
- `EasyEditorPlugin.uplugin`
- `LICENSE`
- `README.md`
- `Resources/Icon128.png`
- `docs/superpowers/plans/2026-07-10-puerts-runtime-plugin.md`
- `docs/superpowers/specs/2026-07-10-puerts-runtime-plugin-design.md`
