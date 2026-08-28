# STM32G431 固件骨架

此目录当前只包含与硬件无关、可在主机上做语法检查的应用层骨架。尚未创建 STM32CubeMX `.ioc`、`Core/` 或 `Drivers/`，因为最终定时器、GPIO、时钟树和板级接口均为 `TBD`。

## 计划结构

```text
firmware/stm32/
├── App/                         应用接口与纯逻辑骨架
├── Core/                        由确认后的 CubeMX 工程生成（尚不存在）
├── Drivers/                     由确认后的 CubeMX 工程生成（尚不存在）
├── README.md
└── hardware_config_template.md
```

## 集成原则

1. 先在独立最小工程验证点灯、UART、单路捕获。
2. HAL 回调只采集时间戳/数据，计算和控制放在周期任务。
3. 所有引脚、定时器实例和 DMA 通道放在板级配置层，不写入通用模块。
4. `app_config.h` 中的默认值不会启用闭环；PPR、增益和超时必须标定。
5. 每个模块通过逻辑分析仪/主机测试独立验收后再集成。

主机语法检查：

```bash
cc -std=c11 -Wall -Wextra -Werror -fsyntax-only firmware/stm32/App/*.c
```

此命令不等同于 STM32 编译或实机测试。
