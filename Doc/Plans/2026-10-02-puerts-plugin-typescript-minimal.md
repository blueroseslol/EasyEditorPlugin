# 插件 TypeScript 最简集成开发计划

状态：用户于 2026-10-03 确认开发，最小集成及 UE 实测已完成，OpenSpec 已归档。实测项目为 `D:/MatrixTA/LyraRPGGameplayAbility/LyraStarterGame.uproject`。

## 基线与目标

- EasyEditor 基线：`889db46828b3f5de5f709fe5922f2b2712486eed`。
- 开发分支：`codex/puerts-plugin-ts-minimal`。
- 不整体移植 `c28bbef9d6e76363e93251017c8ebdd34bf05be8` 或 `41dbfd934a829d733cc3d34f1f46d6135168f86c`。
- UE 插件在自己的 `TypeScript` 目录保存 Runtime TS 源码；修改、新增源码后，编辑器中的 Puerts 自动编译。
- 项目与参与集成的插件共享项目根目录的一个 `tsconfig.json`。
- 优先复用 Puerts 编译、监听及模块加载能力；新增代码优先放在 EasyEditor，必要时允许少量 Puerts 修改。

## 推荐方案

项目根 tsconfig 显式列出项目和插件源码目录，设置 `rootDir: "."`、`outDir: "Content/JavaScript"`，保留现有 Puerts 兼容的其他编译选项。

示例布局：

```text
Project/
  tsconfig.json
  Typing/
  TypeScript/Main.ts
  Plugins/MyPlugin/TypeScript/Runtime/Feature.ts
  Content/JavaScript/
    TypeScript/Main.js
    Plugins/MyPlugin/TypeScript/Runtime/Feature.js
```

插件内及项目到插件的依赖优先使用相对 import。保留 TypeScript 原生输出层级，不改写输出路径，不引入自定义路径别名解析器。复用宿主项目现有 Runtime 环境，不新增 GameInstanceSubsystem、Runtime Host 或独立热重载管理器。

基线 EasyEditor 的编辑器脚本环境与游戏 Runtime 是不同用途。实施时检查两者的入口和输出目录，避免将 Runtime 脚本放到 EasyEditor 编辑器环境执行。项目原有 `Main` 等模块入口可能因输出新增 `TypeScript/` 层级需要迁移，必须列出实际受影响的调用点。

## 首版范围与取舍

1. 先支持项目 `Plugins` 下的真实源码目录；通过 tsconfig 显式指定参与编译的插件。
2. 开发期间保持 Puerts Watch 开启；允许修改 tsconfig 后重启监听或编辑器。
3. 实时编译是必须验收项；运行中热更新沿用 Puerts 现有能力，不承诺所有对象状态和模块副作用无缝更新。
4. 编译产物统一留在宿主项目中。首版不实现插件独立分发、自动打包及产物归属清单。
5. 删除和重命名文件时，验证旧 JS 的实际行为并提供明确清理步骤；不为此建立通用清理框架。
6. 不自动改写用户 tsconfig，不引入 JSONC 编辑器、工作区状态机、多 tsconfig 调度或自定义编译器。
7. 若零代码配置即可满足验收，则交付配置示例、迁移说明及验证证据，不为增加代码而修改 EasyEditor。

## 符号链接与外部目录

实施前确认目标 UE 项目的路径、Puerts 版本以及插件是否通过符号链接或 junction 挂载。当前源码分析显示 Puerts 监听项目目录，但这不能证明链接目标的文件事件能被正确接收。

- 若实际迭代使用链接插件，须在该布局下验证保存和新增文件；失败则视为核心开发场景的缺口，不能只用真实目录的结果宣布完成。
- 优先增加必要目录的监听并复用同一编译服务，统一事件路径与编译输入路径，处理重复事件和监听释放。
- 先检查 EasyEditor 是否能够通过已有接口接入；仅在接口不足时修改 Puerts 的最小相关位置，并同步维护其 TS 源码及实际运行的 JS 产物。
- 任意跨盘源码、引擎级插件的通用支持不作为默认目标。若实际项目必须使用且影响 `rootDir` 或输出结构，应先说明具体限制，再调整方案，不扩展成通用工作区系统。

## 确认后的 OpenSpec 流程

用户确认本文后才执行以下步骤：

1. 检查基线分支的仓库说明、OpenSpec 工具和已有目录状态，保留本地遗留内容及其他工作树。
2. 创建聚焦的变更提案，建议名称 `support-plugin-typescript-minimal`，按实际 OpenSpec 工作流生成 proposal、design、specs 与 tasks。
3. 将本文的范围、迁移代价和验收项转为明确需求，不夹带旧方案的功能集合。
4. 校验 OpenSpec 文档，然后按任务实施；若发现必须扩大已确认范围，先反馈具体证据。
5. 完成验证并如实记录通过项与未验证项，再按实际 OpenSpec 流程完成归档。

## 实施顺序

### 阶段一：确认运行环境与最小验证样例

- 记录 EasyEditor、Puerts 和宿主 UE 项目的实际路径、版本及工作区状态。
- 检查 Watch 开关、根 tsconfig、Runtime 启动方式、声明文件来源和旧输出目录。
- 在合适的验证项目中准备一个项目入口、一个插件模块和一个插件内部依赖。
- 只调整必要配置和引用，先验证 Puerts 原生能力是否足够。

### 阶段二：完成最小集成

- 为实际项目补齐单 tsconfig 的 include、rootDir 和 outDir 配置，保留其他用户选项。
- 调整被输出路径变化影响的 Runtime 入口和模块引用；检查现有 TS 蓝图模块路径，如有使用则纳入迁移。
- 只有阶段一证明存在缺口时，才增加 EasyEditor 接入代码或 Puerts 小补丁。
- 文档化配置方式、目录约定、启用 Watch 和删除旧产物的方法。

### 阶段三：验证与交付

| 验收项 | 通过条件 |
| --- | --- |
| 单配置编译 | 根 tsconfig 同时包含项目与插件源码，无插件独立 tsconfig |
| 首次编译 | 项目入口、插件模块及内部依赖生成到预期目录 |
| 保存重编译 | 编辑器保持开启，修改插件 TS 后 JS 自动更新；错误日志可定位 |
| 新增文件 | 新增并引用插件 TS 后，监听能识别并生成产物 |
| 编译错误恢复 | 引入语法或类型错误能报告，修复后可以继续编译 |
| Runtime 加载 | 游戏 Runtime 能执行项目入口并调用插件导出函数 |
| 路径迁移 | 实际使用的入口和模块引用与新输出路径一致 |
| 编辑器回归 | 基线 EasyEditor 的编辑器脚本启动和重启行为未被破坏 |
| 删除与重命名 | 记录旧产物处理步骤，并验证不会误加载已清理模块 |
| 链接目录（实际使用时必验） | 在真实链接布局中验证保存、新增及路径匹配 |

配置检查、离线编译、UE 编辑器监听、PIE Runtime 加载分别记录证据。未执行 UE 实测不能标记实时编译或 Runtime 加载通过；只在修改 C++ 时安排对应模块构建。首版不把打包运行或任意代码热更新计入已验收能力。

## 交付物

- 聚焦的 OpenSpec 变更及完成情况。
- 可复用的根 tsconfig 配置示例和入口迁移说明。
- 必要且最小的 EasyEditor/Puerts 修改（若原生能力足够，可无代码修改）。
- 实测结果、环境信息和剩余边界。

执行记录见 `openspec/changes/archive/2026-10-03-support-plugin-typescript-minimal/tasks.md`，验证证据见 `Doc/Tests/2026-10-03-plugin-typescript.md`。
