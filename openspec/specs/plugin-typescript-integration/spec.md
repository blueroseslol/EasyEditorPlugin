# plugin-typescript-integration Specification

## Purpose
允许宿主 UE 项目直接编译保存在插件 TypeScript 目录中的 Runtime 源码，并在编辑器保持运行时持续迭代。用户只维护一个根配置，编译产物保持可预测目录结构，由既有 Runtime 加载。
## Requirements
### Requirement: Single project configuration

集成 SHALL 使用宿主项目根目录的一个 tsconfig.json 同时配置项目和所选插件的 Runtime 源码，不要求插件增加独立 tsconfig。

#### Scenario: Compile project and plugin
- **WHEN** 根配置包含项目 TypeScript 目录和指定插件 TypeScript 目录
- **THEN** 两者编译到项目 Content/JavaScript，并保留相对于项目根的目录结构

### Requirement: Continuous plugin compilation

编辑器 Watch 启用后，集成 SHALL 自动编译保存和新增的插件源码；实际采用链接插件时 SHALL 支持该链接布局的迭代。

#### Scenario: Save plugin source
- **WHEN** 用户保存已经纳入编译的插件 TS 文件
- **THEN** 对应 JS 自动更新，无需重新启动编辑器或手动执行构建

#### Scenario: Add plugin source
- **WHEN** 用户在配置覆盖的插件目录新增并引用 TS 文件
- **THEN** 新文件自动编译且引用者可加载其输出

#### Scenario: Recover from compile error
- **WHEN** 用户保存错误的 TS 源码后修复该错误
- **THEN** 编译服务报告错误并在修复后恢复该文件的编译

### Requirement: Runtime loading and editor separation

集成 SHALL 让宿主现有 Runtime 加载项目入口及其插件依赖，并保持 EasyEditor 编辑器脚本用途独立。

#### Scenario: Start gameplay
- **WHEN** 宿主使用迁移后的入口启动 Runtime
- **THEN** 项目代码能够调用插件导出的函数

#### Scenario: Preserve editor tools
- **WHEN** 基线 EasyEditor 启动和重启编辑器脚本
- **THEN** Runtime 的新输出布局不会让编辑器环境误执行游戏入口

### Requirement: Explicit migration and cleanup

交付 SHALL 说明入口路径变化、Watch 前置条件、旧产物清理及运行中热更新的边界，不自动覆盖用户配置或删除业务产物。

#### Scenario: Rename or delete a module
- **WHEN** 用户删除或重命名已编译的插件模块
- **THEN** 文档提供针对对应旧产物的清理步骤，并要求验证旧模块不再被加载
