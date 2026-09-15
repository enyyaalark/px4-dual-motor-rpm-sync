# 固件负责人待做清单

更新时间：2026-09-15

动态状态以 GitHub Issue、PR 和 Project 看板为准；本清单只作为固件负责人的执行入口，不替代 Issue 状态。

## 当前进度判断

截至 2026-09-15，M2 控制链的硬件链路基本打通：

- PWM 输入双路（`PB4/MAIN1`、`PB6/MAIN2`）已实机 `VALID`（400Hz，1000/1100µs）。
- TIM1 双路 PWM 输出已通过逻辑分析仪验证（1000/1140µs、400Hz、同步）。
- HCT157 旁路默认 A 与 3.3V 高选 B 均已 L1 通过；S 在线切换瞬态豁免。
- ESC1/ESC2 无桨启动、停转、失信号停止已定性通过。
- 闭环控制已接入目标固件（`system_controller` 适配层），baseline/sync 两版产物已构建，但尚未在真实台架验证。

主要阻塞：未安装磁体，导致 Hall 双路 RPM 与 P 同步效果无法实测；M1 的停止归零、
最高频率、数字滤波和 RPM 精度证据仍不完整。

## 已完成并合并

- [x] Issue #3：STM32G431 最小点灯、USB DFU、PA9 单向 UART 心跳、冷启动和 30 秒稳定性验证。
  - PR #35 已合并（`e97f635`），Issue #3 已关闭。
  - 实机限制：SWD 已通过 Issue #48 验证；CH340 TXD、5V、3V3、RTS、CTS 保持断开。
- [x] Issue #18：RPM 死区、修正限幅、积分限幅、低速/异常输入保护和条件积分抗 windup。
  - PR #36 已合并（`a98d94c`），Issue #18 已关闭。
- [x] Issue #31：硬件无关 RPM 计算逻辑。
  - PR #32 已复核并合并；主机测试仅为 synthetic 证据，父任务 #7 仍等待实机输入。
- [x] Issue #33：硬件无关 PWM 输入逻辑。
  - PR #34 已复核并合并；未标定 PWM 输入范围保持无效，父任务 #9 仍等待实机输入。
- [x] Issue #7 软件切片：双路 Hall 快照接入 RPM 评估与 v2 UART 状态。
  - PR #66 已合并（`688c3f9`）；#8 参数已进入 main，2026-09-11 已取得双路
    `VALID -> TIMED_OUT` 与 RPM 归零记录，父 Issue 保持 Open/Test 等待证据复核。
- [x] Issue #8 软件切片：PPR/RPM 离线标定分析工具。
  - PR #67 已合并（`8f0bfdb`）；2026-09-10 实机数据确认 `PPR=1`，但父 Issue 仍缺
    最高频率/滤波和可追溯的 3% 精度结论。
- [x] Issue #10 软件切片：双路 PWM 输出 C/C++ 薄适配边界。
  - PR #68 已合并（`f2f711b`）；父 Issue 保持 Open + blocked，ESC 参数为 `TBD`，未生成 TIM1 配置。
- [x] Issue #9 实机：双路 PWM 输入捕获已验证；MAIN1 从 `PA6` 改为 `PB4`（PR #78）。
- [x] Issue #10 实机：TIM1 双路输出逻辑分析仪验证通过。
- [x] Issue #11 实机：HCT157 默认 A 与高选 B L1 通过；S 切换瞬态豁免。
- [x] Issue #12 实机：ESC1/ESC2 无桨功能验证。
- [x] 闭环集成准备（未提交，待验证）：`system_controller` 适配层、台架输出边界/增益、`main.c` 控制步、`rpm_sync_ctrl,v1` 遥测；baseline/sync 两版 Release 产物已构建。

## 当前可以继续的小项

- [x] 闭环集成准备（已完成，待台架验证后提交）。
- [ ] 待磁体安装后：验证双路 Hall RPM，采开环基线与 P 同步对比。
- [ ] 闭环验证通过后，把闭环源码整理为 `feat:` PR。
- [ ] Issue #14 固定 11 字段 CSV：本轮推迟，演示使用 `rpm_sync_capture,v2`、`rpm_sync_pwm_input,v1`、`rpm_sync_ctrl,v1`。
- [ ] 保持主机侧测试可重复；维护 UART 校验工具与日志格式检查。

## 等待硬件条件后再做

- [x] Issue #6：按修订范围关闭；L1 UART 与双路串扰证据已具备，手动周期对照取消，最高频率/滤波结论转交 Issue #8。
- [x] Issue #7：使用 Issue #8 参数构建/刷写当前 main/v2，并记录双路同时 `VALID`、
  停转后 `TIMED_OUT` 及 RPM 归零；精确 100 ms 时刻和异常上限实机注入仍未验证。
- [ ] Issue #7/#8：等待磁体安装后做 RPM/PPR 收口。
- [ ] Issue #16：P 同步实机验证（等待磁体）。
- [ ] Issue #19/#20：故障、复位、掉电和旁路恢复；等待可控测试条件。
- [ ] Issue #21：带桨台架测试（已移出本轮压缩范围）。
- [ ] Issue #22/#23：对比曲线、指标和最终报告；等待真实台架 CSV。

## 固件侧安全约束

- 闭环默认关闭；除已确认的 `PPR=1` 外，增益、超时、死区和修正限幅未经标定不得启用。
- 未确认的 GPIO、定时器、DMA、协议、电压和参数保持 `TBD`。
- 霍尔、PWM、HCT157 和电机测试必须遵守 `docs/safety.md`；当前仍是 L1 逻辑验证。
