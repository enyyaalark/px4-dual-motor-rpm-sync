# 接线说明

> 本文是逻辑接线计划，不是最终针脚表。所有 MCU 引脚、电压和器件电源脚必须依据实物数据手册与 `pin-map.md` 复核。

## 霍尔通道（每路）

```text
3.3V ---- 4.7kΩ ----+---- A3144E OUT
                    |
                    +---- 1kΩ ----+---- SN74HC14 IN
                                   |
                                  1nF
                                   |
                                  GND

SN74HC14 OUT --------------------------- STM32 TIMx_CHy (TBD)
```

1nF 是起始值；只有根据示波器/逻辑分析仪观察到的脉宽、边沿和干扰，才考虑 10nF。过大电容会吞掉高速脉冲。

## HCT157 旁路

```text
PX4 PWM1 ------ A1      Y1 ------ ESC1 signal
STM32 PWM1 ---- B1

PX4 PWM2 ------ A2      Y2 ------ ESC2 signal
STM32 PWM2 ---- B2

STM32 BYPASS/SELECT ---- S
固定使能电平 ---------- /E
```

默认电阻必须保证 STM32 复位、掉电或 GPIO 高阻时选择 A（PX4 原始输出）。具体 S 极性、`/E` 逻辑、供电电压和未使用输入处理应以 SN74HCT157N 数据手册和实测为准，不能凭器件名称推断。

## UART

```text
STM32 TX ---- USB-TTL RX（第一阶段）
STM32 GND --- USB-TTL GND
```

第一阶段只要求单向遥测。当前 CH340 USB-TTL 的 TXD 空闲高电平实测为 5V，因此其 TXD、5V、3.3V、RTS 和 CTS 均不得连接 STM32；只允许在 L1 条件下连接 STM32 TX 到 USB-TTL RXD，并共地验证接收。若连接 PX4，必须确认对应 UART 电平和端口用途，并开发明确的解析模块。

## Issue #3 最小下载与点灯接线

2026-09-01 已在本项目使用的 WeAct QFN48 V1.0 实板确认：核心板 USB-C 数据口可进入 STM32 ROM DFU（USB VID:PID `0483:df11`）。在断开电机、电调、电池、CH340 和 ST-LINK 后，按住 `BOOT`、短按并松开 `RESET`、等待约 1 秒再松开 `BOOT`，可由 STM32CubeProgrammer 通过 USB 下载。下载必须启用写后校验；正常启动或冷启动时不得按住 `BOOT`。

该验证仅证明此实板的 USB DFU 下载路径可用，不代表应用固件实现了 USB CDC，也不替代后续 SWD 调试能力。Issue #3 固件经 USB DFU 写入校验后，PC6 LED 的 500 ms 翻转在复位启动和 USB-C 断电重启后均已观察通过。`PA9/USART1_TX -> CH340 RXD` 在 115200 8N1 下首次与 USB-C 断电约 5 秒后再次采集均得到至少 3 条完整、格式匹配的 `MONITOR_ONLY` 心跳。

Linux 主机需要为 `0483:df11` 安装 udev 规则后才能下载。默认该设备属于 `root:root`，普通用户只有读权限，此时 `lsusb` 能看到设备，但 STM32CubeProgrammer 报 `No STM32 device in DFU mode connected`。规则文件只新增 `0483:df11`，不修改 ST-LINK 现有规则；规则生效后必须重新插拔并重新执行按键顺序，因为 udev 不对已连接设备追溯。

下载后必须冷启动或按 `RESET` 再判断点灯结果，不能以 `STM32_Programmer_CLI -g` 的跳转启动为准。ROM DFU bootloader 为运行 USB 已启用 HSI48 与 PLL，而 `SystemClock_Config` 按复位后状态配置时钟（`PLL.PLLState = RCC_PLL_NONE`），跳转后时钟配置可能失败并停在 `Error_Handler()` 的 `__disable_irq()` 死循环，现象是状态灯保持熄灭。该现象 2026-09-01 已实测复现，冷启动后点灯恢复正常。

需要 SWD 调试或 USB DFU 不可用时，由核心板 USB-C 提供逻辑电源，ST-LINK 只连接三根调试信号。下面的 `CLK`、`DIO` 和 `GND` 均在核心板 100mil 排针孔位上；排针焊接和三线 SWD 链路已按 Issue #48 完成验证，但每次重新接线仍须复核供电隔离和信号对应关系。

```text
ST-LINK SWCLK ---- 核心板 CLK（PA14）
ST-LINK SWDIO ---- 核心板 DIO（PA13）
ST-LINK GND   ---- 核心板 GND
```

Issue #48 已于 2026-09-04 完成上述三线链路验证：STM32CubeProgrammer 连续两次只读识别结果一致，目标电压 3.27 V、Device ID `0x468`、设备族 STM32G43x/G44x、NVM 128 KBytes；断电拆除三线并仅恢复核心板 USB-C 后，原 PC6 点灯固件正常运行。该结果只证明本次连接下的 SWD 调试路径，不授权连接 ST-LINK 供电脚，也不证明其他 GPIO 或控制链已经验证。

按 ST-LINK 外壳丝印识别信号，不凭连接器朝向猜测针脚。验证时不连接 ST-LINK 的 3.3V、5V、RST、SWIM，也不连接 CH340；这样核心板只有 USB-C 一条供电路径。上电前先断开电机、电调和电池，检查 3.3V 对 GND 无明显短路，并由另一人复核三根线。下载完成并断电后，才进入上面的单向 UART 接线步骤。

## 接线复核清单

- [ ] 螺旋桨已拆除，电池未连接
- [ ] 所有芯片型号、方向、电源脚和地脚已核对
- [ ] 逻辑电源电压和绝对最大额定值已核对
- [ ] PX4、STM32、逻辑芯片、ESC 信号共地
- [ ] 主电流路径未经过面包板、洞洞板或杜邦线
- [ ] HCT157 默认选择状态用万用表/逻辑分析仪验证
- [ ] 每个逻辑芯片有 100nF 就近退耦
- [ ] 两人交叉核对后才上电
