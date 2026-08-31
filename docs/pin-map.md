# 引脚映射（待确认）

在飞控型号、电调和 PCB/洞洞板布局确认前，不创建 `.ioc`，不猜测 STM32 引脚。

| 功能 | 方向 | 外部信号 | STM32 定时器/GPIO | 电气要求 | 状态 |
|---|---|---|---|---|---|
| Hall 1 capture | 输入 | HC14 OUT1 | `TBD` | 定时器输入捕获 | 待分配 |
| Hall 2 capture | 输入 | HC14 OUT2 | `TBD` | 定时器输入捕获 | 待分配 |
| PX4 PWM 1 | 输入 | PX4 motor output 1 | `TBD` | 标准 PWM，电平待测 | 待分配 |
| PX4 PWM 2 | 输入 | PX4 motor output 2 | `TBD` | 标准 PWM，电平待测 | 待分配 |
| ESC PWM 1 | 输出 | HCT157 B1 | `TBD` | 1000–2000µs 暂定 | 待分配 |
| ESC PWM 2 | 输出 | HCT157 B2 | `TBD` | 1000–2000µs 暂定 | 待分配 |
| Bypass select | 输出 | HCT157 S | `TBD` | 复位默认 PX4 直通 | 待分配 |
| Telemetry TX | 输出 | CH340 RXD（第一阶段） | `PA9 / USART1_TX` | 3.3V UART；仅单向连接 | Issue #3 bring-up 已分配，最终控制器待复核 |
| Telemetry RX | 输入 | 第一阶段不连接 | `PA10 / USART1_RX` | CH340 TXD 实测 5V，禁止直连 | 仅为 USART1 初始化保留 |
| Status LED | 输出 | 板载蓝色 LED | `PC6` | WeAct QFN48 V1.0，低速推挽输出 | Issue #3 bring-up 已确认 |

## 分配准则

1. 两路霍尔应使用有足够计数范围和输入捕获能力的定时器通道。
2. 两路 PX4 PWM 捕获和两路 ESC PWM 输出避免通道/复用冲突。
3. 确认引脚为 3.3V 容限、上电状态安全且不占用 SWD。
4. 记录 CubeMX 版本、时钟树、计数频率和溢出处理策略。
5. 分配完成后由另一名成员对照原理图、数据手册和核心板丝印复核。
