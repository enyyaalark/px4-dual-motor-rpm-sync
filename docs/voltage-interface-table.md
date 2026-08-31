# 电压接口表

> 建立日期：2026-08-31
> 状态：部分硬件型号已确认，接口电平仍待实测
> 原则：未确认项保持 `TBD`，不猜测；每项确认都要附数据手册、照片或测量证据

## 表格

| 接口 | 发送端 | 发送端电平/协议 | 接收端 | 接收端要求 | 实测值 | 状态 | 证据 |
|---|---|---|---|---|---|---|---|
| PX4 -> STM32 PWM 输入 | Pixhawk 6C Mini，固件版本 `TBD` | 标准 PWM；PWM 信号电压可硬件切换 3.3V/5V | STM32G431 定时器输入捕获 | 3.3V 容限 | `TBD` | 型号已确认，实际输出电平待确认 | Holybro 官方文档 |
| STM32 -> HCT157 | STM32G431 PWM 输出 | 3.3V PWM | SN74HCT157N B1/B2 | 输入高电平阈值按数据手册 | `TBD` | 待测 | 数据手册/逻辑波形 |
| HCT157 -> ESC | SN74HCT157N Y1/Y2 | 标准 PWM，暂定 1000–2000µs | Raptor 5 ESC，固件 `Flycolor_Raptor_5` | 支持常规 1–2ms PWM；3.3V 控制电平待实测 | `TBD` | 型号已确认，3.3V 接受度待实测 | Flycolor 说明书/实测 |
| A3144E -> HC14 | A3144E OUT，开集电极 | 3.3V 上拉 4.7kΩ，1kΩ 串联，1nF 起始滤波 | SN74HC14N 施密特输入 | 施密特输入，3.3V 逻辑 | `TBD` | 待测 | `docs/wiring.md`/逻辑波形 |
| HC14 -> STM32 霍尔捕获 | SN74HC14N OUT | 3.3V 整形方波 | STM32 定时器输入捕获，引脚 `TBD` | 定时器输入捕获能力 | `TBD` | `TBD` | 引脚分配后确认 |
| STM32 UART -> PC/PX4 | STM32G431 TX | 3.3V UART，波特率 `TBD` | USB-TTL RX / PX4 RX | RX 电平匹配 | `TBD` | 待测 | 串口记录 |
| 接收机 -> PX4 遥控输入 | FLYSKY FS-SR8 | ANT 协议；数据输出支持 PWM/PPM/i.BUS/s.BUS | Pixhawk 6C Mini RC 输入 | 待确认使用哪种输出模式 | `TBD` | 型号已确认，输出模式待定 | FS-SR8 说明书 |
| 电池 -> ESC 主电源 | 3S LiPo，4000mAh，标称 11.1V | 11.1V 标称电压 | Raptor 5 ESC | 支持 3–6S | `TBD` | 电池和 ESC 电压范围已确认，实际值待测 | 电池标签/Flycolor 说明书 |
| 电池 -> 降压模块 -> 逻辑电源 | 电池，规格已记录 | 11.1V 标称电压 | 降压模块，型号 `TBD` | 输入额定范围 | `TBD` | `TBD` | 数据手册/万用表 |
| 降压模块 -> 逻辑电路 | 降压模块输出，型号 `TBD` | 输出电压 `TBD`，额定电流 `TBD` | STM32/HC14/HCT157 供电 | 3.3V 逻辑电源，纹波 `TBD` | `TBD` | `TBD` | 数据手册/万用表 |
| 共地 | PX4、STM32、HC14、HCT157、ESC 信号地 | GND | 所有控制信号共同参考地 | 共地 | `TBD` | 待测 | 接线复核记录 |

## 当前已从仓库确认的信息

- A3144E 输出为开集电极，按设计使用 `4.7kΩ` 上拉至 3.3V，`1kΩ` 串联，`1nF` 起始滤波，再进入 SN74HC14N。
- STM32G431 应用层按 3.3V 逻辑设计，UART 输出为 3.3V 电平。
- HCT157 暂定使用 STM32 的 3.3V PWM 作为 B 输入，默认选择 PX4 原始 PWM。

## 本次新增确认信息

- Pixhawk 6C Mini 的 PWM 输出支持 3.3V 和 5V 两种硬件切换模式，需要检查实际板子焊接了哪个电压。
- Raptor 5 ESC 支持常规 1–2ms 标准 PWM 输入，上电时自动检测输入协议。
- Raptor 5 ESC 支持 3–6S 电池，与 3S 4000mAh 11.1V 电池匹配。
- FLYSKY FS-SR8 使用 ANT 协议，数据输出支持 PWM、PPM、i.BUS 和 s.BUS，尚未确定 PX4 使用哪一种。

## 仍需补充确认的信息

- Pixhawk 6C Mini 固件版本
- Pixhawk 6C Mini 实际 PWM 输出电平是 3.3V 还是 5V
- Raptor 5 ESC 对 3.3V 控制电平的实际接受度
- FS-SR8 最终使用 PWM、PPM、i.BUS 还是 s.BUS 输出
- 降压模块型号、输入电压范围、输出电压和额定电流
- SN74HC14N 和 SN74HCT157N 的供电电压方案
- STM32 定时器/GPIO 最终分配

上述信息确认后，再逐项把 `TBD` 替换为实测值和证据。

## 参考来源

- Holybro Pixhawk 6C Mini PWM 信号电压说明：`https://docs.holybro.com/autopilot/pixhawk-6c-mini/pwm-signal-voltage-mod.md`
- Flycolor Raptor 5 说明书及第三方规格页
- FlySky FS-SR8 规格页
