# 固件负责人待做清单

更新时间：2026-09-01

动态状态以 GitHub Issue、PR 和 Project 看板为准；本清单只作为固件负责人的执行入口，不替代 Issue 状态。

## 已完成，等待 PR 合并

- [x] Issue #3：STM32G431 最小点灯、USB DFU、PA9 单向 UART 心跳、冷启动和 30 秒稳定性验证。
  - PR #35：<https://github.com/enyyaalark/px4-dual-motor-rpm-sync/pull/35>
  - 实机限制：SWD 尚未验证；CH340 TXD、5V、3V3、RTS、CTS 保持断开。
- [x] Issue #18：RPM 死区、修正限幅、积分限幅、低速/异常输入保护和条件积分抗 windup。
  - PR #36：<https://github.com/enyyaalark/px4-dual-motor-rpm-sync/pull/36>
  - 这是基于 PR #35 的 stacked PR，#35 合并后需将基线切回 `main`。
- [x] Issue #31：硬件无关 RPM 计算逻辑（PR #32，等待合并）。
- [x] Issue #33：硬件无关 PWM 输入逻辑（PR #34，等待合并）。

## 当前可以继续的小项

- [ ] 检查 PR #35/#36 的 Review 意见并按要求修改。
- [ ] PR #35 合并后，将 PR #36 rebase/retarget 到 `main`，重新运行 CI。
- [ ] 保持主机侧 RPM、PWM、同步控制边界测试可重复；新增测试必须直接覆盖实际 C++ 实现。
- [ ] 维护 UART 校验工具和日志格式检查；实机原始记录只新建、不覆盖。
- [ ] 根据硬件负责人提供的确认资料更新 `docs/open-questions.md`、`docs/pin-map.md` 和配置参数。

## 等待硬件条件后再做

- [ ] Issue #6：双路定时器输入捕获；等待最终 GPIO/定时器分配及霍尔整形波形。
- [ ] Issue #7：RPM 实机接入、异常检测和停止归零；等待双路捕获与 PPR 标定。
- [ ] Issue #9：PX4 标准 PWM 输入捕获；等待 PX4 型号、固件和实测 PWM 波形。
- [ ] Issue #10：双路 PWM 输出；等待 ESC 电平兼容性、PWM 范围和最终引脚。
- [ ] Issue #14：完整 UART CSV 遥测；等待 RPM/PWM/状态机数据源接入。
- [ ] Issue #16：P 同步控制实机验证；必须先有开环基线、PPR 和安全条件。
- [ ] Issue #19/#20：故障、复位、掉电和旁路恢复；等待旁路硬件和可控测试条件。
- [ ] Issue #22/#23：对比曲线、指标和最终报告；等待真实台架 CSV。

## 固件侧安全约束

- 闭环默认关闭；PPR、增益、超时、死区和修正限幅未经标定不得启用。
- 未确认的 GPIO、定时器、DMA、协议、电压和 PPR 保持 `TBD`。
- 霍尔、PWM、HCT157 和电机测试必须遵守 `docs/safety.md`；当前仍是 L1 逻辑验证。
