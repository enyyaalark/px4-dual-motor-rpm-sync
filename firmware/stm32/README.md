# STM32G431 固件骨架

此目录包含与硬件无关、可在主机上检查的 C++17 应用层骨架、Issue #3 的最小点灯/UART 工程，以及 Issue #6 的双路霍尔捕获候选配置。PWM、旁路和最终整机落针仍未验证；当前工程不能作为最终控制器配置。

`rpm_sync_bringup.ioc` 是 Issue #3 专用的最小点灯/UART 配置，不是最终控制器引脚表。它只使用经 WeAct Studio V1.0 原理图和官方示例确认的板级资源：QFN48 `STM32G431CBU6`、`PC6` 用户 LED、`PA13/PA14` SWD，以及 STM32 数据手册支持的 `PA9/PA10` USART1。后续霍尔、PWM、旁路和 DMA 分配仍保持 `TBD`。

`rpm_sync_capture.ioc` 在该基线上加入 Issue #6 候选捕获：`PA0/TIM2_CH1` 和 `PA1/TIM2_CH2` 共用 1 MHz、32 位自由运行计数器，双路均为上升沿直接输入、中断捕获。数字滤波暂为 `0`，必须根据 HC14 实际波形和最高预期频率再确定。`hall_capture.c` 的中断路径只记录捕获 tick、周期和毫秒时间戳；主循环每秒通过 PA9 输出一次 `rpm_sync_capture,v1` 遥测，包含两路有效标志、周期（µs）和最后脉冲年龄（ms）。RPM 换算仍由周期任务完成，且 PPR 保持 `TBD`。

## 计划结构

```text
firmware/stm32/
├── App/                         C++17 应用接口与纯逻辑骨架
├── rpm_sync_bringup.ioc         Issue #3 最小点灯/UART 配置
├── rpm_sync_capture.ioc         Issue #6 双路 Hall 捕获候选配置
├── Inc/                         CubeMX 生成的初始化头文件
├── Src/                         CubeMX 生成的初始化源码和启动心跳
├── Drivers/                     STM32CubeG4 v1.6.3 所需 HAL/CMSIS 子集
├── STM32CubeIDE/                IDE 工程、启动文件和链接脚本
├── README.md
└── hardware_config_template.md
```

## 集成原则

1. 先在独立最小工程验证点灯、UART、双路捕获。
2. CubeMX 生成的 HAL/Core 可以保留 C；在薄的 `.cpp` 适配层中实现回调，必要时用 `extern "C"` 导出 HAL 要求的符号。
3. HAL 回调只采集时间戳/数据，计算和控制放在周期任务。
4. 所有引脚、定时器实例和 DMA 通道放在板级配置层，不写入通用模块。
5. `app_config.hpp` 中的默认值不会启用闭环；PPR、增益和超时必须标定。
6. 每个模块通过逻辑分析仪/主机测试独立验收后再集成。

主机语法检查：

```bash
c++ -std=c++17 -Wall -Wextra -Werror -fsyntax-only firmware/stm32/App/*.cpp
```

此命令不等同于 arm-none-eabi-g++ 交叉编译或 STM32 实机测试。嵌入式构建建议关闭 RTTI 和异常，但需在生成工程后通过尺寸与行为验证再固定编译选项。

## 点灯与 UART 启动前检查

先检查 CubeMX、CubeIDE、交叉编译器、下载器和调试器是否可发现：

```bash
python3 tools/check_stm32_toolchain.py
```

脚本会同时检查 `PATH` 和 STM32CubeIDE 的标准 Linux 安装目录。默认不打印本机绝对路径；`--show-paths` 只用于本地诊断，输出不得复制到仓库、Issue 或测试记录。

当前最小工程固定使用：

- WeAct Studio STM32G431 QFN48 V1.0、`STM32G431CBU6`；
- HSI 16 MHz、`PC6` 状态灯、`PA9/USART1_TX`、115200 8N1；
- STM32CubeMX 6.18.1、STM32CubeG4 v1.6.3、STM32CubeIDE 工程；
- 每 500 ms 翻转 LED，每 1 s 发送 `rpm_sync_bringup,v1,...,mode=MONITOR_ONLY`。

在 STM32CubeIDE 中导入 `firmware/stm32/STM32CubeIDE`。重新生成前必须安装 STM32CubeG4 v1.6.3，并选择只复制所需库文件；不得把本机固件包绝对路径写入 `.ioc`。

`Debug` 和 `Release` 两个 configuration 均已启用 `convertbinary`，构建后除 `.elf` 还产出 `.bin`。USB DFU 下载只接受 `.bin`，因此该选项不能关闭；`tests/test_stm32_bringup_config.py` 对此有回归测试，防止重新生成工程时被覆盖。DFU 下载使用 `Release/rpm_sync_bringup.bin`。

无图形界面时可用 headless 构建：

```bash
/path/to/stm32cubeide/headless-build.sh -data <临时工作区> \
  -importAll firmware/stm32/STM32CubeIDE -cleanBuild all
```

临时工作区不要放在仓库内。`Debug/`、`Release/`、`*.elf`、`*.bin` 已被 `.gitignore` 覆盖。

UART 仅允许单向连接：核心板 `PA9` 接 CH340 模块 `RXD`，两者 `GND` 相连。已测得 CH340 `TXD` 空闲电平约 5 V，因此 `TXD`、`5V`、`3V3`、`RTS`、`CTS` 均不得连接核心板。

本实板已于 2026-09-01 确认可通过核心板 USB-C 进入 STM32 ROM DFU，并使用 STM32CubeProgrammer 完成固件写入和校验。PC6 LED 的 500 ms 翻转已通过复位启动及 USB-C 断电重启观察。PA9 单向 UART 首次采集和 USB-C 断电约 5 秒后的第二次采集均获得至少 3 条完整、格式匹配的心跳；另外 30 秒连续采集获得 10/10 条有效心跳。Issue #48 于 2026-09-04 完成三线 SWD 只读验证：两次目标识别结果一致，断开 ST-LINK 后原点灯固件正常恢复；未执行擦除、下载或选项字节修改。

点灯与 UART 属于 L1 测试：电机主电源必须断开，螺旋桨必须拆除，并避免 USB、ST-LINK 和外部电源之间反灌。
