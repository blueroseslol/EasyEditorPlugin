# 插件 TypeScript 验证记录

日期：2026-10-03（Asia/Singapore）。结果：本次最小集成范围的验收通过。

## 环境

- EasyEditor：`889db46828b3f5de5f709fe5922f2b2712486eed`，分支 `codex/puerts-plugin-ts-minimal`。
- Puerts：`815a3504375af8132d1f3a1b1f98bba8cf85e621`，新增同名分支；仅修改 CodeAnalyze TS 及生成的 JS/map。原有 JsEnv.Build.cs、PEBlueprintAsset.h 修改未纳入本次补丁。
- 宿主：`D:/MatrixTA/LyraRPGGameplayAbility/LyraStarterGame.uproject`，HEAD `c9caa9e2ef0a068239540101939657962706b77a`，原分支 master。
- 引擎：`D:/UnrealEngine/UE_5.8`，UE 5.8.2。
- Puerts 和 EasyEditor 均通过宿主 Plugins 下的符号链接接入。
- Puerts 编译器：TypeScript 4.7.4；宿主 CLI：4.9.5。
- UE 实测启动使用 `-unattended -nullrhi -nosound`、`/Engine/Maps/Entry`，通过 Python 调用实际 PIE 和编辑器重启，不属于视觉验收。

## 结果

| 验证 | 证据与结果 |
| --- | --- |
| 迁移前检查 | 旧 TypeScript/tsconfig.json 用项目 CLI `--noEmit` 通过 |
| 单根配置 | 迁移后 TS 4.9.5 与 Puerts 所用 4.7.4 均通过；插件无独立 tsconfig |
| 构建 | LyraGame 入口与基线 EasyEditorPlugin 模块构建通过，UBT `Result: Succeeded` |
| 原生监听缺口 | 10:44 在插件物理目录改为 WatchBaseline，输出仍为 Hello；同一编辑器里项目内新增 TS 正常生成输出 |
| 链接目录修复 | 10:49:03 在插件物理目录保存，输出自动变成 WatchFixed；不执行 CLI 构建、不重启编辑器 |
| 新增及新子目录 | AddedProbe.ts 和 ProbeSubdir/Nested.ts 自动生成 JS；随后在插件入口引用，生成结果可加载两者 |
| 编译错误 | 对 AddedProbe 引入 string→number 类型错误，UE 明确记录源码位置并保留上一份有效输出 |
| 错误恢复 | 修复后输出更新为 recovered；由已生成插件入口调用的离线断言通过 |
| 重命名 | AddedProbe→RenamedProbe 自动编译；确认旧 AddedProbe.js 仍存在，按文档停止编辑器后逐文件清理 |
| 首次 UE 编译 | 删除本次插件样例 JS/declaration/map 和本次生成的版本缓存后启动 UE，10:55:02 自动重新生成 Greeting/index；期间未执行 tsc 发射 |
| Runtime 加载 | 实际 LyraGameInstance 创建的 JsEnv 加载 TypeScript/Main；10:55:55 输出 `[EasyEditor Runtime],Hello, Lyra!` |
| 编辑器独立运行 | 基线编译出的 EasyEditorPlugin 加载 Content/EasyEditorScripts/Main.js；EasyEditor.Restart 输出 cleanup 后再次 startup |
| 清理后校验 | 新 Node 进程确认旧探针模块 MODULE_NOT_FOUND；最终 TypeScript 4.7.4 `--noEmit` 通过 |
| 生成产物一致性 | TS 4.7.4 重新 transpile 的 CodeAnalyze.js/map 与 Puerts 仓库产物一致，且与宿主部署副本一致 |
| 差异与规范 | 针对本次文件的 diff 检查通过（仓库 CRLF 使用 cr-at-eol）；OpenSpec 严格校验通过 |

## 修改范围

EasyEditor 只增加 Runtime TS 样例、配置示例和文档，未修改 C++ 或插件模块布局。

Puerts 的源码补丁为 CodeAnalyze.ts 新增 18 行、删除 7 行：提取同一个监听回调，按解析后的 include 根目录注册 watcher，复用旧 watcher 时解除旧监听和回调。生成 JS 因原仓库 TS/JS 不同步而恢复了一些既有日志和错误诊断；其额外差异不是新增编译框架。

Lyra 迁移现有配置到根目录，构建命令指向根配置，Runtime 入口改为 TypeScript/Main，加入对插件样例的引用。新增 DefaultPuerts.ini、EasyEditor 链接和最小编辑器入口。编译器 npm 依赖已在项目部署目录安装，保留生成的 package-lock.json。

原有游戏 Main/UI 产物未删除，现有 WaitDebugger 未修改。PIE 测试通过本机 8889 inspector 的 Runtime.runIfWaitingForDebugger 放行；用户后续仍需按原方式连接调试器。

## 本地证据

目录：`D:/MatrixTA/LyraRPGGameplayAbility/Saved/EasyEditorMinimal/2026-10-03/`。

- `before-status.txt` 与 `before/`：开始时状态，以及本次迁移涉及的原始配置、入口等文件副本。
- `build.log`：模块构建结果。
- `editor.log`：原生监听对照及基线编辑器重启。
- `editor-fixed.log`：链接目录、新增、错误恢复、重命名及第一次 PIE。
- `editor-final.log`：清除样例产物后的首次编译、最终 PIE 和正常退出。
- `probe.py`：本地 Python 控制入口；只在测试进程中注册，进程已退出。

这些本地证据不作为插件运行依赖。测试 TS 探针及其输出已删除，样例 Greeting/index 已恢复。

## 明确边界

- 未验收打包、独立分发、Linux/macOS 链接或任意项目外目录。
- 未迁移业务 TS 蓝图资产；已有资产模块路径要独立核对。
- 未实现自建 JsEnv 的运行中热更新，验收的是保存自动编译以及重新开始 PIE 加载。
- 删除/重命名后需要精确清理旧产物并重启编译器；没有引入自动清理框架。
- include 必须列出具体链接插件源码根，宽泛 Plugins/** 不能保证跨链接事件。
- UE 构建仍有原有 API 废弃及 UHT 目录警告，不影响本次构建通过。
