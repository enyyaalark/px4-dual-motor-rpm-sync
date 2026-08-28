# 硬件配置记录模板

> 复制本模板时不得填写账号、个人路径、串口设备名或密钥。

## 版本

- 日期：`TBD`
- PX4 型号/固件：`TBD`
- STM32 核心板版本：`TBD`
- ESC 型号/固件：`TBD`
- 电池：`TBD`
- 固件提交：`TBD`

## 时钟与引脚

| 功能 | GPIO | AF | 定时器/通道 | 计数频率 | 复位状态 | 证据 |
|---|---|---|---|---|---|---|
| Hall 1 | TBD | TBD | TBD | TBD | 输入 | TBD |
| Hall 2 | TBD | TBD | TBD | TBD | 输入 | TBD |
| PX4 PWM 1 | TBD | TBD | TBD | TBD | 输入 | TBD |
| PX4 PWM 2 | TBD | TBD | TBD | TBD | 输入 | TBD |
| ESC PWM 1 | TBD | TBD | TBD | TBD | 安全/旁路 | TBD |
| ESC PWM 2 | TBD | TBD | TBD | TBD | 安全/旁路 | TBD |
| HCT157 select | TBD | TBD | GPIO | n/a | PX4 直通 | TBD |
| UART TX | TBD | TBD | TBD | n/a | 高阻 | TBD |

## 标定参数

- `pulses_per_revolution`: `TBD`（必须有独立转速计证据）
- PWM 最小/启动/最大：`TBD`
- 霍尔超时：`TBD`
- 控制周期：`TBD`
- `Kp`, `Ki`, deadband, correction limit：`TBD`

## 复核

- [ ] 数据手册与核心板原理图一致
- [ ] CubeMX 无引脚/时钟冲突
- [ ] 复位时 HCT157 默认 PX4 直通
- [ ] 逻辑电平和共地已实测
- [ ] 第二名成员签字复核
