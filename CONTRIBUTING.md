# 贡献指南

本项目由两名成员协作，默认建议角色为 `role:hardware` 与 `role:firmware`，不绑定具体 GitHub 用户名。

## 工作方式

1. 从 Issue 开始，确认输入、前置任务和安全条件。
2. 从 `main` 创建短功能分支，例如 `docs/pin-map`、`feat/rpm-capture`。
3. 一次 Pull Request 只解决一个可验证目标。
4. PR 中填写测试方法、证据、风险和未验证项；至少由另一名成员复核。
5. 合并前更新对应文档、测试记录和 `CHANGELOG.md`。

推荐提交信息：

```text
docs: add system architecture and safety plan
feat: add rpm capture module skeleton
test: add synthetic rpm log plot
```

## 硬件变更规则

- 不猜测引脚、电压、电调协议或每转脉冲数；未知内容写 `TBD`。
- 原理图、接线图或针脚表必须有日期/版本，并由另一人交叉复核。
- 实验记录必须包含硬件配置、固件提交、测试条件、CSV 原始数据和结论。
- `data/raw/` 保存不可改写的原始日志；处理结果写入 `data/processed/`。
- 主电流线、电池与螺旋桨相关操作必须遵守 `docs/safety.md`。

## 软件质量

- STM32 应用层使用 C++17；生成的 HAL C 代码只通过明确的适配边界进入应用层。
- 参数只放在 `app_config.hpp` 或未来的持久化配置层。
- 每个模块先独立验证，再接入完整状态机。
- 未经实机验证的代码必须明确标注，不能宣称已测试。
- 不提交密钥、账号、个人路径、串口设备名、设备序列号或真实凭据。

## Pull Request 检查

- [ ] Issue 和目标明确
- [ ] 无敏感信息、机器路径和未知硬件假设
- [ ] 测试步骤可重复，结果与证据链接完整
- [ ] 安全影响已复核
- [ ] 文档、配置和变更日志已同步
