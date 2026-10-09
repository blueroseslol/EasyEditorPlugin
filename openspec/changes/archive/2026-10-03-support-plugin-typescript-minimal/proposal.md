## Why

Runtime TypeScript 需要保留在 UE 插件仓库中并随保存自动编译。此前方案引入多配置、工作区管理和 Runtime 框架，超过当前需要；本次从 `889db468` 建立可实测的最小集成。

## What Changes

- 提供单一根 tsconfig 的配置样例、插件 Runtime 样例与接入说明。
- 使用 Puerts 原有编译服务和项目 JavaScript 输出目录。
- 在用户指定的 Lyra 项目验证真实目录及链接插件的持续编译，必要时仅补足目录监听。
- **BREAKING**：采用项目根 rootDir 后，项目脚本输出增加 `TypeScript/` 层级，宿主入口需要同步迁移。
- 不增加多根模块加载器、自定义 emit、Runtime Host、自动 tsconfig 编辑或打包系统。

## Capabilities

### New Capabilities

- `plugin-typescript-integration`：项目与插件通过一个 tsconfig 编译、插件修改自动生成 JS，并由宿主 Runtime 加载。

### Modified Capabilities

无。

## Impact

- EasyEditor：配置样例、插件内 TS 样例、接入说明及必要的最小接入代码。
- 验证项目：`D:/MatrixTA/LyraRPGGameplayAbility` 的 TS 配置、构建命令和 Runtime 模块入口。保留已有 UI 与其他未提交修改。
- Puerts：先复用现有代码，只有实测确认监听缺口才补丁；不得混入该仓库已有 C++ 修改。
