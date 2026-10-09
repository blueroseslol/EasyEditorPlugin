## Context

EasyEditor 基线只有编辑器 JsEnv，不管理项目 Runtime。Puerts 的 CodeAnalyze 读取项目根 tsconfig、监听项目目录、通过 TypeScript LanguageService 发射代码。DefaultJSModuleLoader 能从项目 Content/JavaScript 解析保持原层级的输出。

验证项目当前只有 TypeScript/tsconfig.json，通过外部 tsc 编译；LyraGameInstance 自建 JsEnv 并启动 Main。TypeScript 配置、Main、GameInstance 和 UI 产物已有用户未提交修改，需要按文件内容局部迁移。项目 Puerts 为符号链接，拟接入的 EasyEditor 也以链接方式使用，以验证真实迭代场景。

## Goals / Non-Goals

目标：插件 TypeScript 保存后编译，单 tsconfig，现有宿主 Runtime 可加载结果。

非目标：重建运行时生命周期；保证任意正在运行对象热更新；插件独立分发；自动清理所有历史输出；修改用户业务 UI。

## Decisions

1. **保留原生目录层级**：rootDir 为项目根，outDir 为 Content/JavaScript；跨项目/插件使用相对引用。避免 import 重写及别名运行时映射。
2. **显式配置**：include 只列出参与的插件目录；TS 目录名为 TypeScript。相比自动扫描，可避免禁用插件、测试代码和 node_modules 混入编译。
3. **复用 Puerts Watch**：UE 对照测试证明项目根监听可收到项目内新增文件，但收不到链接插件的保存事件。Puerts 无现成的编译刷新接入接口，因此在 CodeAnalyze 中增加 tsconfig 显式 include 根目录的监听，复用同一回调、编译服务及输出路径。修改 TS 源码并重新生成 JS/map；原先生成文件缺失的编译诊断也随生成恢复。
4. **宿主局部迁移**：将现有配置按新位置调整相对路径，保留其他选项；构建命令指向根配置；GameInstance 入口改为 TypeScript/Main。任何调试等待仅在验证时处理，不默认删除用户设置。
5. **分层验证**：离线 TS 编译与 CommonJS 解析验证只能证明输出和依赖；UE Watch、PIE 和 EasyEditor 编辑器环境分别提供证据。

## Risks / Trade-offs

- 链接目录事件与源码路径可能不一致 → 在外部真实目录修改文件，检查 UE 自动输出，不凭离线编译认定通过。
- 根目录迁移可能改变模块名 → 检查入口、相对依赖、第三方模块和历史 JS；不删除用户已有产物。
- 项目自建 JsEnv 不一定接收 Puerts 默认环境刷新 → 首版承诺编译和下一次启动加载，热更新单独标注。
- 本地 DLL 可能来自旧分支 → 实测记录二进制来源；涉及 C++ 或需要验证基线行为时构建当前源码。
- 新旧 TypeScript 版本不同 → 使用实际 Puerts 编译器版本做兼容检查。

## Migration Plan

保存改动前文件副本和状态到本地验证目录；显式接入插件目录；迁移配置与入口；完成离线检查后启动指定 UE 项目做监听测试。回退仅恢复本次更改的片段和新增文件，不 reset、stash 或清理用户工作区。
