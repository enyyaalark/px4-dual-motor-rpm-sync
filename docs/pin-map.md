# 引脚映射（Issue #6 捕获配置已生成，整机方案仍待验证）

Issue #6 的候选方案已通过成员 A 的 L0 排针可及性复核和 STM32CubeMX 6.18.1 生成检查。双路 Hall 捕获配置已落入 `rpm_sync_capture.ioc`；PWM、旁路及最终整机落针仍须各自完成 CubeMX、电气和实机验证，因此本表不代表完整控制器已经定版。

| 功能 | 方向 | 外部信号 | STM32 定时器/GPIO | 电气要求 | 状态 |
|---|---|---|---|---|---|
| Hall 1 capture | 输入 | 第一颗 HC14 `1Y` | `PA0 / AF1 / TIM2_CH1` | 3.3 V 整形输出；上升沿捕获 | L0 可及性、CubeMX 与物理接线已复核；捕获波形待测 |
| Hall 2 capture | 输入 | 第二颗 HC14 `1Y` | `PA1 / AF1 / TIM2_CH2` | 3.3 V 整形输出；上升沿捕获 | L0 可及性、CubeMX 与物理接线已复核；捕获波形待测 |
| PX4 PWM 1 | 输入 | Pixhawk MAIN1（左侧后推电机基础指令） | 候选 `PA6 / AF2 / TIM3_CH1` | 标准 PWM，电平待测 | 逻辑通道已确认；电气与排针待复核 |
| PX4 PWM 2 | 输入 | Pixhawk MAIN2（右侧后推电机基础指令） | 候选 `PB6 / AF2 / TIM4_CH1` | 标准 PWM，电平待测 | 逻辑通道已确认；电气与排针待复核 |
| ESC PWM 1 | 输出 | HCT157 B1 | 候选 `PA8 / AF6 / TIM1_CH1` | 实际范围和频率 `TBD` | 待排针、波形与 ESC 边界复核 |
| ESC PWM 2 | 输出 | HCT157 B2 | 候选 `PA10 / AF6 / TIM1_CH3` | 实际范围和频率 `TBD` | 待排针、波形与 ESC 边界复核 |
| Bypass select | 输出 | HCT157 S | 候选 `PB2 / GPIO` | 复位高阻，由外部电阻保证 PX4 直通 | S 极性、电平和排针待复核 |
| Telemetry TX | 输出 | CH340 RXD（第一阶段） | `PA9 / USART1_TX` | 3.3V UART；仅单向连接 | Issue #3 bring-up 已实板验证，最终控制器待复核 |
| Telemetry RX | 输入 | 第一阶段不连接 | 不分配；候选方案将 `PA10` 用于 `TIM1_CH3` | CH340 TXD 实测 5 V，禁止直连 | 最终控制器不启用 RX；bring-up `.ioc` 保持原样 |
| Status LED | 输出 | 板载蓝色 LED | `PC6` | WeAct QFN48 V1.0，低速推挽输出 | Issue #3 bring-up 已确认 |

## 定时器资源与理由

| 定时器 | 候选用途 | 计数方案 | 设计理由 | 未验证项 |
|---|---|---|---|---|
| `TIM2` | Hall 1/2 上升沿捕获 | CubeMX 配置为 1 MHz 自由运行、32 位递增计数 | 两路共享同一时间基准；1 µs/tick 时约 71.6 分钟回绕，应用层按无符号 32 位差值处理单次回绕 | 实机 tick/波形对照、输入滤波、最大脉冲频率和中断负载 |
| `TIM3` | PX4 PWM 1 测量 | 候选 1 MHz；PWM input/reset mode | 每路 PWM 独占一个定时器，避免两路信号争用同一 slave-reset 时间基准 | PX4 实际频率、电平、脉宽及 CubeMX 生成配置 |
| `TIM4` | PX4 PWM 2 测量 | 与 TIM3 相同 | 与 MAIN1 独立测量，可分别检测超时和越界 | 同上 |
| `TIM1` | ESC PWM 1/2 输出 | 候选 1 MHz；CH1/CH3 preload，同一 update event 生效 | 两路共享周期计数器并同步更新；最终周期和限幅等待 ESC 实测 | 启动/复位瞬态、输出电平、频率、脉宽和 HCT157 波形 |

## Issue #6 实际 Hall 接线

成员 A 于 2026-09-03 更正确认：两路分别使用两颗独立 SN74HC14N 的第一组门，而不是在同一颗芯片上使用 `1A/1Y` 和 `2A/2Y`。

```text
第一颗 SN74HC14N：1A 接第一路 RC 输出，1Y -> PA0/TIM2_CH1
第二颗 SN74HC14N：1A 接第二路 RC 输出，1Y -> PA1/TIM2_CH2
```

两颗芯片的独立 RC 网络、3.3 V 供电、100 nF 退耦和共地已由成员 A 复核。门编号变化不改变 STM32 引脚或固件通道映射；上电电平、双路捕获结果和逻辑分析仪周期对照仍未验证。

`PA11/PA12` 保留给板载 USB/ROM DFU 路径，不用于控制输出；`PA13/PA14` 始终保留 SWD；`PA9` 保留单向遥测；`PC6` 保留状态灯。`PA10` 在 Issue #3 bring-up 工程中仍是未接线的 USART1_RX，但第一阶段最终控制器不需要 RX，因此候选方案可将其改作 `TIM1_CH3`。这不授权连接 CH340 TXD。

## HAL 捕获边界

1. `HAL_TIM_IC_CaptureCallback` 或等效 IRQ 适配层只读取捕获寄存器、记录通道和时间戳，并调用有界的脉冲采集入口；不在中断中计算 RPM、判断故障、格式化 CSV 或发送 UART。
2. Hall 两通道读取同一 `TIM2` 32 位计数域；不得用毫秒系统时钟代替捕获 tick。应用层周期任务再调用 `evaluateRpm` 处理超时、异常和归零。
3. PX4 PWM 每路使用独立定时器测量周期和高电平宽度；输入极性、数字滤波和超时在真实波形可用后确定，当前不填写魔法数。
4. 捕获数据从 IRQ 交给周期任务时必须使用固定大小快照和明确的临界区/原子策略；不得动态分配、阻塞或使用无界队列。
5. 首轮实机验证只允许 L1 手动脉冲或已复核的逻辑信号。电机主电源断开、螺旋桨拆除；逻辑分析仪物理接线和原始波形采集由成员 A 完成。

## 复核依据

- STMicroelectronics [`DS12589 Rev 6`](https://www.st.com/resource/en/datasheet/stm32g431cb.pdf)，STM32G431x6/x8/xB 数据手册 Table 12/13：封装引脚和 alternate function。
- STMicroelectronics [`RM0440 Rev 9`](https://www.st.com/resource/en/reference_manual/dm00355726.pdf)：`TIM2` 为 32 位通用定时器，`TIM3/TIM4` 为 16 位通用定时器；输入捕获、PWM input 和 preload/update 行为以该手册为准。
- WeAct Studio [`WeActStudio.STM32G431CoreBoard`](https://github.com/WeActStudio/WeActStudio.STM32G431CoreBoard) 官方仓库的 QFN48 V1.0 原理图：候选 GPIO 已引出到两侧排针；最终仍须成员 A 对实板丝印和可及性复核。

## 分配准则

1. 两路霍尔使用同一个 32 位自由运行时间基准，以保证跨通道周期差可直接比较。
2. 两路 PX4 PWM 捕获和两路 ESC PWM 输出避免通道/复用冲突。
3. 确认引脚为 3.3V 容限、上电状态安全且不占用 SWD。
4. 记录 CubeMX 版本、时钟树、计数频率和溢出处理策略。
5. 分配完成后由另一名成员对照原理图、数据手册和核心板丝印复核。

第一阶段固定使用 `MAIN1 -> 左侧后推电机`、`MAIN2 -> 右侧后推电机`，并只验证两路基础指令相同的同步。这里的编号从机体向前观察，以机体左/右为准；它只是逻辑通道基线。在确认 Pixhawk 硬件修订、输出电平、连接器方向并完成无动力波形验证前，不得据此连接 ESC。
