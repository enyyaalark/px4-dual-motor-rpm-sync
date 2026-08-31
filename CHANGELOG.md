# Changelog

本项目遵循“未验证不宣称”的原则。日期使用 `YYYY-MM-DD`。

## [Unreleased]

### Added

- 项目架构、硬件、接线、控制、UART、安全、校准、测试、计划、BOM、风险和开放问题文档。
- STM32G431 模块接口骨架与集中配置模板。
- synthetic/demo CSV、RPM 日志校验/绘图工具和基础测试。
- GitHub Issue/PR 模板、标签、里程碑与三周任务规划。
- AI 项目上下文、任务防重复规则和 Issue/Project 进度交接规范。
- 硬件无关 RPM 状态计算与 C++17 主机测试，覆盖首脉冲、32 位回绕、超时归零、无效配置和异常脉冲隔离。

### Changed

- STM32 应用层技术栈确定为 C++17，模块扩展名改为 `.cpp/.hpp`。
- 增加 C++17 主机语法检查，并明确 CubeMX HAL C 与应用层的 ABI 适配边界。
- RPM 采集状态不再用零时间戳表示“尚无脉冲”；诊断原始 RPM 与可供后续控制使用的有效 RPM 分离。

### Not verified

- 所有硬件、电气、PWM、RPM、旁路和闭环控制行为均尚未实机验证。
- RPM 主机测试仅使用 synthetic 参数；最终 PPR、计时器频率、超时、最大 RPM、异常跳变阈值和实机误差仍未验证。
