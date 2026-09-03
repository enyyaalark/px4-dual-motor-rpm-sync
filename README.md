# 基于 PX4 与 STM32 的双电机转速监控及同步控制系统

> PX4 + STM32G431 dual-motor RPM monitoring and bounded synchronization controller using Hall sensors.

本项目面向双发后推固定翼的台架验证结构：PX4 负责遥控输入、姿态与上层飞行控制；STM32G431 独立测量两台无刷电机的实际转速，在两个基础指令相同的前提下实施有限幅的同步修正，并通过 UART 输出可复核数据。

**当前状态：规划、代码骨架、STM32 最小 bring-up，以及 PA0/PA1 + TIM2 的双路 Hall 捕获软件基线已完成；成员 A 已交接两颗 HC14 到 PA0/PA1 的 L1 接线。双路捕获波形、PPR/RPM、PWM、旁路和电机实机验证仍未完成，不具备安全飞行条件。**

## 项目目标

- 用两路外置 A3144E 霍尔传感器稳定测量 RPM，并用独立转速计标定每转有效脉冲数。
- 捕获 PX4 标准 PWM 基础指令，生成两路受限修正 PWM。
- 先完成监控，再逐步启用 P/可选 PI 同步控制。
- 通过 HCT157 在 STM32 未就绪时默认选择 PX4 原始输出。
- 用 CSV 日志和可重复测试证明结果，而不是凭主观观察判断。

## 系统框图

```text
遥控器 -> 接收机 -> PX4 -> 基础 PWM -----> HCT157 A 输入
                         |                  |
                         +-> STM32G431 -----+-> HCT157 -> ESC1/ESC2 -> 电机1/电机2
                              ^       |
                 霍尔1/霍尔2 --+       +-> UART CSV -> 电脑（后续可接 PX4 解析模块）
```

霍尔信号链：

```text
A3144E -> 4.7kΩ 上拉至 3.3V -> 1kΩ 串联 -> 1nF 起始滤波
        -> SN74HC14 施密特整形 -> STM32 定时器输入捕获
```

详细设计见 [系统架构](docs/architecture.md)、[接线说明](docs/wiring.md)和[控制算法](docs/control-algorithm.md)。

## 硬件清单

- Pixhawk 6C Mini 飞控（PX4 v1.12.3；硬件修订 `TBD`）、FlySky FS-SR8 接收机（ANT 协议；实际输出模式 `TBD`）
- STM32G431CBU6 WeAct 核心板 ×2、ST-LINK V2、USB 转 TTL
- RS2205 2300KV 无刷电机 ×2、Flycolor Raptor5 G071-35A 同型号电调 ×2（用户确认两只一致，固件标识均为 `Flycolor_Raptor_5`）
- 3S1P 电池（用户确认 4000mAh、额定 11.1V、100C；化学体系、满充/当前实测电压、连接器和线规 `TBD`）
- A3144E ×5（2 使用、3 备用）
- SN74HC14N、SN74HCT157N
- 4.7kΩ、1kΩ、10kΩ 电阻；1nF、10nF、100nF、10–47µF 电容
- 8 通道 24MHz 逻辑分析仪、面包板、双面洞洞板
- UL2547 三芯 28AWG 屏蔽软线、3D 打印霍尔支架
- 已有电池、降压模块、焊台、万用表、电机和电调

完整数量和用途见 [BOM](docs/bill-of-materials.md)。

## 软件组成

```text
firmware/stm32/       CubeMX/HAL 工程、双路 Hall 捕获与 C++17 应用层
tools/                CSV 校验和绘图工具
tests/                主机侧基础测试
docs/                 架构、接线、算法、安全、校准、计划与风险文档
```

STM32 应用层使用 C++17，状态机至少包括 `INIT`、`MONITOR_ONLY`、`SYNC_CONTROL`、`BYPASS`、`FAULT`。参数集中在 `app_config.hpp`。Issue #6 已生成 `PA0/TIM2_CH1`、`PA1/TIM2_CH2` 双路 Hall 捕获配置；PWM、旁路和最终整机资源仍保持候选或 `TBD`。CubeMX 生成的 HAL C 代码通过薄适配层接入，HAL 回调只采集固定大小数据。

## 快速开始

文档和工具可立即使用；Issue #3 的最小点灯/UART 工程及 Issue #6 的双路 Hall 捕获工程均可构建。捕获工程尚待刷写和 L1 实机验收，不能作为最终控制器配置：

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r tools/requirements.txt
python3 tools/plot_rpm_log.py data/demo/synthetic_rpm_log.csv --output data/processed/demo_plot.png
python3 -m unittest discover -s tests -v
c++ -std=c++17 -Wall -Wextra -Werror -fsyntax-only firmware/stm32/App/*.cpp
```

示例 CSV 是明确标记的 `synthetic/demo` 数据，不能作为硬件性能证据。

## 调试顺序

1. 确认飞控、电调、接收机、电池和协议，完成电压/接口复核。
2. STM32 点灯与 UART；此时不连接电机主电源。
3. 手动移动磁体验证单路霍尔、上拉、滤波和 HC14 波形。
4. 标定每转有效脉冲数，再完成双路输入捕获和 RPM 归零。
5. 验证 PX4 PWM 输入与 STM32 PWM 输出，先接逻辑分析仪或示波器。
6. 验证 HCT157 上电、复位和旁路默认状态。
7. 拆桨完成全链路与日志测试。
8. 采集开环基线，逐步启用小增益 P 控制；PI 只有在数据支持时才加入。
9. 满足 [安全计划](docs/safety.md) 后才允许刚性台架带桨测试。

## UART 数据格式

```text
timestamp_ms,base_pwm_us,rpm1,rpm2,error_rpm,error_percent,correction_us,pwm1_us,pwm2_us,system_state,fault_flags
```

UART 第一阶段发送到电脑；PX4 不会自动理解这些数据。若未来要供飞控内部使用，必须另行开发解析模块。详见 [UART 协议](docs/uart-protocol.md)。

## 安全警告

- **初期调试必须拆除螺旋桨。**
- 带桨测试必须使用刚性固定台架、防护罩和独立急停。
- 人员不得位于桨盘平面内；上电前必须口头确认警戒区清空。
- 电池和电调主电流不得经过面包板、洞洞板、杜邦线或本项目信号线。
- PX4、STM32、电调控制信号必须共地，但供电路径和电压必须人工复核。
- 接线和供电必须由两人交叉复核。
- 本项目当前是实验原型，不是飞行认证系统，也不能替代飞控安全机制。

## 三周里程碑

| 里程碑 | 截止日期 | 验收目标 |
|---|---:|---|
| M1 双路 RPM 测量 | 2026-09-04 | 稳定显示两台电机 RPM |
| M2 完整控制链 | 2026-09-11 | 遥控控制两台无桨电机并记录双路 RPM |
| M3 同步控制与验证 | 2026-09-18 | 数据显示同步开启后的稳态转速差明显降低 |

工作日安排见 [项目计划](docs/project-plan.md)；固件负责人执行入口见 [固件待做清单](docs/firmware-task-checklist.md)。周六、周日不安排任务。

## 当前完成状态

- [x] 项目范围、安全边界、架构和三周计划
- [x] 文档、Issue/PR 模板和 STM32 C++17 模块骨架
- [x] synthetic/demo CSV 绘图工具与主机侧基础测试
- [x] WeAct 核心板 USB DFU、PC6 点灯与 PA9 单向最小 UART 心跳实机验证
- [x] 双路 Hall 候选引脚、CubeMX 配置、HAL 捕获和 UART 校验工具
- [ ] 双路 Hall 无脉冲/单路/同时触发及逻辑分析仪周期对照
- [ ] 硬件型号、电压、协议和引脚确认
- [ ] 霍尔、PWM、旁路及完整 UART CSV 遥测实机验证
- [ ] 开环基线、同步控制和故障注入测试
- [ ] 带桨台架数据、演示视频和最终报告

## 已知限制

- 飞控外壳型号、PX4 v1.12.3、接收机型号（FS-SR8；旧照片识读 FS-iA10B 已更正）、双发后推布局、两只相同电调（固件标识均为 `Flycolor_Raptor_5`）以及 3S1P/11.1V/4000mAh/100C 电池额定信息已确认；当前 `SYS_AUTOSTART=1001`（HIL Quadcopter X）与目标不一致。Pixhawk 6C Mini 硬件修订、接收机实际输出模式、电池化学体系/实测电压和螺旋桨仍未确认；STM32 Hall 捕获已分配 PA0/PA1，其他整机引脚仍待验证。
- A3144E 能否隔着电机外壳稳定检测磁场尚未验证。
- 第一版只研究两个基础指令相同时的同步，不覆盖 PX4 主动差动推力。
- 第一版旁路主要覆盖 MCU 复位或未启动，不能覆盖全部程序锁死故障。
- UART 只是观测通道；CAN/DroneCAN 和 PX4 解析模块不在三周核心范围内。

## 下一阶段计划

- 根据台架数据决定是否保留 PI、增加独立硬件看门狗。
- 为 PX4 开发明确版本绑定的 UART 解析/记录模块。
- 评估 CAN/DroneCAN、目标差动转速接口和冗余故障检测。
- 在任何飞行考虑前开展独立安全评审、环境测试与失效分析。

## 协作

使用 `main` 作为唯一长期分支；每项任务使用短功能分支和 Pull Request。暂不添加许可证。贡献要求见 [CONTRIBUTING.md](CONTRIBUTING.md)。AI 助手开始工作前应读取 [AGENTS.md](AGENTS.md)，任务状态和交接方式见[进度记录规范](docs/progress-tracking.md)。
