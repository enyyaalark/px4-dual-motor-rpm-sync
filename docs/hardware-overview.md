# 硬件概览

## 已有硬件

| 类别 | 器件 | 数量/状态 | 备注 |
|---|---|---:|---|
| 控制 | STM32G431CBU6 WeAct 核心板 | 2 | 一块主用，一块备用/并行验证 |
| 飞控 | Pixhawk 6C Mini | 1，照片与 USB/MAVLink 确认 | PX4 v1.12.3；制造商/硬件修订仍为 `TBD`；当前 HIL 四旋翼机架配置不适用于本项目 |
| 遥控 | FlySky FS-SR8 接收机 | 1，用户确认（旧照片识读 FS-iA10B 已更正） | 厂家资料为 2.4GHz ANT 协议（适配 FS-ST8/FS-ST16 等 ANT 遥控），8 通道、双天线、供电 3.5–9V，输出可选 PWM/PPM/i-BUS/s-BUS；实际绑定状态与输出模式 `TBD`（不能由型号推断） |
| 动力 | RS2205 2300KV 电机 | 2 | 不在面包板上承载主电流 |
| 动力 | Flycolor Raptor5 G071-35A 电调 | 2，用户确认同型号，固件标识均为 `Flycolor_Raptor_5` | 厂家规格 3–6S、35A 持续/40A 10 秒、无 BEC；固件版本号、配置和 3.3V 输入兼容性待确认 |
| 供电 | 3S1P 电池 | 1，用户确认 3S1P/11.1V/4000mAh/100C | 化学体系、满充电压、当前实测电压、连接器和线规待确认；容量×C 换算的理论电流仍待标签/实测复核 |
| 传感 | A3144E 数字霍尔开关 | 5 | 2 使用、3 备用 |
| 整形 | SN74HC14N | 1+ | 3.3V 供电能力需按实物数据手册确认 |
| 选择 | SN74HCT157N | 1+ | 供电、电平阈值和默认选择需台架验证 |
| 工具 | ST-LINK V2、CH340 系列 USB-TTL、逻辑分析仪、万用表 | 已有 | USB-TTL 枚举为 `1a86:7523`，具体 CH340 后缀 `TBD`；逻辑分析仪 8 通道 24MHz |

## 电源域与接地

- 电池只进入动力配电/电调主电源，不进入面包板或杜邦线。
- 降压模块为控制电子设备供电；输出电压、额定电流、纹波和上电顺序 `TBD`。
- PX4、STM32、HCT157、HC14 与电调控制信号必须具有共同参考地。
- USB、ST-LINK、外部电源同时连接前检查地环路和反灌风险。
- 每个逻辑芯片就近放置 100nF 退耦，板级电源入口放置 10–47µF 滤波。

## 接口确认表

| 接口 | 发送端电平/协议 | 接收端要求 | 状态 |
|---|---|---|---|
| PX4 -> STM32 | 台架逻辑映射为 MAIN1=左侧后推、MAIN2=右侧后推；当前参数为 MAIN 1–4、400Hz、全局 1075–1950us；实际电平和波形待测 | 3.3V 容限和定时器捕获 | 左右通道已确认，但参数只说明配置意图；厂家资料显示不同修订可硬件选择 3.3V/5V，禁止仅凭参数接线 |
| STM32 -> HCT157 | 3.3V PWM | HCT157 输入高阈值 | 待数据手册/实测 |
| HCT157 -> ESC | 标准 1–2ms PWM（Raptor 5 厂家手册支持） | ESC 控制高阈值仍 `TBD` | 协议能力已查证；3.3V 高电平兼容性和实际校准范围待实测 |
| A3144E -> HC14 | 开集电极 + 3.3V 上拉 | HC14 施密特输入 | 待台架波形 |
| STM32 -> CH340 USB-TTL | 3.3V UART TX，115200 8N1（Issue #3 bring-up） | CH340 RXD | L1 单向心跳与断电重启后采集已实测通过 |
| CH340 USB-TTL -> STM32 | TXD 空闲高电平实测 5V | 仅在增加合规电平转换后连接 | 阻塞；第一阶段保持断开 |
| STM32 -> PX4 | 3.3V UART，波特率 `TBD` | PX4 RX 电平/协议 | 后续阶段待确认 |

## 2026-09-01 照片识别记录

| 临时照片 | 可确认内容 | 不能由照片确认 |
|---|---|---|
| `7f6325c0951edf70ecb6a66b5a37cfc5.png` | 用户确认照片为外壳底面；可读到 `pixhawk 6c mini`，以及 MAIN 1–8、AUX 1–4、AUX5、AUX6 和 RC IN | 该底面没有可读的制造商、Model A/B/legacy 修订或 PWM 电压焊盘状态；照片也不能确认 PX4 固件版本和参数 |
| `8331a393389e3762310df1afeb5515b0.png` | FlySky `FS-iA10B`，外壳标出 SERVO、SENS、B/VCC、BIND 和 PPM/i-BUS 相关端口 | 当前绑定状态、实际选择的 PWM/PPM/i.bus/s.bus 输出模式和输出电平 |
| `d761b629ed0799d37f86dbc523ed2c4a.png` | 一只 Flycolor Raptor 5、35A、3–6S 单体电调；用户后续确认第二只同型号 | 照片本身不能证明第二只一致；两只实刷固件版本、配置参数、输入阈值和失联行为仍待确认 |

> 更正（2026-09-02）：用户确认接收机一直为 FlySky FS-SR8；上表 2026-09-01 依照片识读的 `FS-iA10B` 是错误识别，不作为当前硬件基线。FS-SR8 官方资料见下方厂家链接；实际输出模式不能由型号推断。

照片位于 `.gitignore` 忽略的临时 `Pictures/` 收件箱，不作为可长期访问的仓库证据。型号规格另由厂家资料交叉核对：

- Pixhawk 6C Mini 文档：<https://docs.holybro.com/autopilot/pixhawk-6c-mini>
- FlySky FS-SR8 官方产品页：<https://www.flyskytech.com/parts_detail/113.html>
- FlySky FS-SR8 官方用户手册（PDF）：<https://flyskytech.com/u_file/photo/20250326/FS-SR8%20User%20manual%2020240619.pdf>
- Flycolor Raptor5 G071 产品页：<https://www.fly-color.net/index.php?c=category&id=234>
- Flycolor Raptor 5 厂家手册：<https://en.fly-color.net/uploadfile/202209/88d64251ad.pdf>

## 2026-09-01 PX4 USB/MAVLink 只读检查

测试条件：仅 Pixhawk USB 连接电脑供电；电池、接收机、ESC、电机、STM32、ST-LINK 和 CH340 均断开。检查只读取 USB 描述符、`AUTOPILOT_VERSION` 和参数，没有写参数、刷写固件或重启飞控。

| 项目 | 读取结果 | 解释/限制 |
|---|---|---|
| USB | `3185:0038 Auterion PX4 FMU v6C.x`，`/dev/ttyACM0` | 确认 FMUv6C 家族；不能区分 Pixhawk 6C Mini 的 Model A/B/legacy 修订 |
| 固件 | PX4 `1.12.3` | 通过 MAVLink `AUTOPILOT_VERSION` 确认；未记录设备 UID |
| 机架 | `SYS_AUTOSTART=1001`，`MAV_TYPE=2` | PX4 v1.12 文档将 1001 定义为 HIL Quadcopter X，与本项目双发固定翼目标不一致 |
| 仿真开关 | `SYS_HITL=0` | HITL/SIH 未启用；不能消除上述错误机架配置风险 |
| RC 输入 | `COM_RC_IN_MODE=0` | 配置为实体 RC 发射机输入；不能证明 FS-SR8 当前使用哪种输出模式 |
| IO | `SYS_USE_IO=1` | 使用 IO 板输出路径 |
| MAIN PWM | `PWM_MAIN_RATE=400`、`PWM_MAIN_OUT=1234` | 参数表示 MAIN 1–4 为 ESC 输出、400Hz；尚未用逻辑分析仪验证实际波形 |
| MAIN 范围 | 全局 min 1075us、max 1950us、disarmed 900us | MAIN1/2 独立 min/max 均为 -1，即继承全局值；这些值尚未与 Raptor5 实机校准 |
| AUX PWM | `PWM_AUX_OUT=0` | 当前未配置 AUX ESC 输出 |

安全阻塞：在单独评审并更正 PX4 机架与输出配置、确认 PWM 电平并用逻辑分析仪验证之前，不连接 ESC、电机或电池。PX4 v1.12 参数含义参考：<https://docs.px4.io/v1.12/en/advanced_config/parameter_reference>；机架 1001 参考：<https://docs.px4.io/v1.12/en/airframes/airframe_reference>。

台架通道决策：第一阶段使用 `MAIN1 -> 左侧后推电机`、`MAIN2 -> 右侧后推电机`，两路采用相同基础指令。左右以机体向前观察时的机体左/右为准。该决策不选择最终飞行机架，也不解除上述动力连接阻塞。

## 机械安装

验证对象为双发后推布局。霍尔支架应可调位置、避免接触转子、承受振动且不会脱落进入桨盘。磁体方案、传感距离和固定方式必须先在拆桨/低能量条件下验证；该布局确认不等于螺旋桨尺寸、旋向、间距或防护方案已经确认。
