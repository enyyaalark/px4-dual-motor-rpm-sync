# 系统架构

## 范围与边界

第一阶段只做三周双发后推布局台架验证结构。PX4 保持上层控制权，STM32 负责快速测速、有限同步修正、故障判断、旁路请求和 UART 遥测。第一阶段仍只验证两个基础指令相同的同步，不推导或替代未来飞行中的差动推力逻辑；系统不宣称具备飞行安全性。

```text
[RC] -> [Receiver] -> [PX4]
                         | MAIN1=left / MAIN2=right
                         | equal base PWM in phase 1
                         v
Hall 1 -> HC14 -> +-------------------+ corrected PWM 1/2
Hall 2 -> HC14 -> | STM32G431         |------------------+
                  | capture/control   |                  |
                  | fault/telemetry   |                  v
                  +-------------------+          +---------------+
                            | select ------------>| HCT157        |
                            |                     | A=PX4, B=STM32|
                            +-> UART CSV          +-------+-------+
                                                           |
                                                       ESC1 / ESC2
```

左右以机体向前观察时的机体左/右为准。第一阶段逻辑映射固定为 `MAIN1 -> 左侧后推电机`、`MAIN2 -> 右侧后推电机`；这不代表当前 PX4 机架或电气接口已经验证。

## 职责划分

| 组件 | 负责 | 不负责 |
|---|---|---|
| PX4 | 遥控输入、姿态和上层飞控、基础电机目标 | 自动解析自定义 UART CSV、快速霍尔测速 |
| STM32 | PWM 捕获、RPM、同步修正、故障和遥测 | 替代完整飞控、未经授权覆盖差动推力 |
| HCT157 | 在原始与修正 PWM 之间进行硬件选择 | 覆盖所有 MCU 锁死或输出粘连故障 |
| PC 工具 | 记录、校验、绘图和离线比较 | 实时安全控制 |

## STM32 模块

- `rpm_capture`：时间戳/周期捕获与 RPM 更新。
- `pwm_input`：PX4 标准 PWM 测量及输入有效性。
- `pwm_output`：最终 PWM 限幅和双路输出接口。
- `sync_controller`：死区、P/可选 PI、修正限幅与抗饱和。
- `hall_monitor`：脉冲超时、异常值和归零。
- `fault_manager`：锁存/非锁存故障与状态迁移。
- `bypass_control`：自检前默认直通、旁路请求。
- `telemetry_uart`：固定字段 CSV 输出。
- `system_controller`：组合上述模块，执行周期采样、故障刷新、状态迁移和遥测组装。

## 状态机

```text
INIT --自检通过--> MONITOR_ONLY --人工使能且条件满足--> SYNC_CONTROL
 |                       |                                |
 +--异常--> FAULT <------+----------异常------------------+
     |                                                      
     +--安全策略--> BYPASS <----复位/未就绪/人工请求---------+
```

- `INIT`：输出旁路选择，初始化捕获、PWM、UART 和故障管理。
- `MONITOR_ONLY`：测量并记录，不施加修正。
- `SYNC_CONTROL`：仅在双路输入有效、RPM 高于阈值且基础目标允许时闭环。
- `BYPASS`：选择 PX4 原始 PWM；仍尽量输出故障遥测。
- `FAULT`：停止积分和修正，根据故障策略转入旁路或安全状态。

### 硬件无关状态机组合

`system_controller` 是应用层的硬件无关组合层：HAL/中断层先把双路霍尔脉冲写入两个 `RpmCapture`、把 PX4 脉宽写入 `PwmInput`，周期任务再调用 `step(controller, now_ms, dt_seconds)` 得到最新 `AppState`、旁路选择、双路 PWM 和可直接交给 `telemetry_uart` 的 `TelemetrySample`。

它按下面顺序刷新，并保持“修正 PWM 只有在自检完成、无人工旁路、无故障且输入有效时才可选”的不变量：

1. 计算两路 `RpmReading` 和 `PwmInputReading`；
2. 把霍尔超时、异常脉冲和 PWM 输入无效刷新为活动故障；
3. 用 `bypass_control` 和 `fault_manager` 计算修正 PWM 是否允许；
4. 仅当修正允许、人工使能、基础 PWM 有效且两路 RPM 均高于最小闭环转速时启用 `sync_controller`；
5. 以 `base_pwm - correction` 和 `base_pwm + correction` 通过 `pwm_output` 限幅，基础 PWM 无效时两路输出为 0；
6. 生成遥测样本并更新 `INIT/MONITOR_ONLY/SYNC_CONTROL/BYPASS/FAULT` 状态。

旁路选择信号等于第 3 步的门控结果，与是否进入 `SYNC_CONTROL` 无关；`MONITOR_ONLY` 中修正量为 0，因此选择“修正 PWM”等同于透传基础指令。

当前版本只完成主机侧逻辑验证。故障锁存策略、`kOutputSaturated` 与旁路门控的关系、`FAULT -> BYPASS` 的具体迁移，以及低速 `error_percent` 哨兵仍保持 `TBD`，需要在对应 Issue 中评审；未接入 HAL、定时器、DMA 或 UART 发送层，不能据此声称实机状态机已工作。

### 硬件无关旁路门控

修正 PWM 只有在自检完成、没有人工旁路请求且 `fault_manager` 没有活动或锁存故障时才可被选择。复位必须同时清除“自检完成”并恢复人工旁路请求，因此即使故障管理器已清零也不能在复位后自动选择修正 PWM。

非锁存故障清除后，可以在其他门控条件仍满足时恢复修正；锁存故障即使清除 active 位也继续强制旁路，只有显式复位故障管理器才清除。旁路门控结果必须作为 `sync_controller` 的使能输入；门控关闭时修正和积分都归零。

上述规则的主机测试只验证应用层逻辑，不证明 HCT157 上电默认电平、实际切换波形或 STM32 掉电行为。硬件仍必须通过逻辑分析仪完成 T08/T13。

## 时间与数据流

硬实时捕获在定时器/中断层记录时间戳；周期任务计算 RPM 和故障；控制周期读取同一快照并产生受限修正；低优先级 UART 发送日志。具体频率在测得脉冲范围和 CPU 负载后确定，当前为 `TBD`。

## 后续扩展

PX4 若主动要求差动推力，控制目标应改为保持 `target_rpm1 - target_rpm2`，而不是强制相等。CAN/DroneCAN、PX4 解析模块、独立硬件看门狗属于后续阶段。
