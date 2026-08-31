# 数据工具

`plot_rpm_log.py` 校验 UART CSV，并绘制：

1. RPM1/RPM2 随时间变化；
2. 转速差；
3. 转速差百分比；
4. 修正量；
5. 两路 PWM。

安装与运行：

```bash
python3 -m pip install -r tools/requirements.txt
python3 tools/plot_rpm_log.py data/demo/synthetic_rpm_log.csv --validate-only
python3 tools/plot_rpm_log.py data/demo/synthetic_rpm_log.csv --output data/processed/demo_plot.png
```

`data/demo/synthetic_rpm_log.csv` 是人工构造的格式演示数据，不代表真实硬件、控制效果或验收结果。

真实实验日志应写入 `data/raw/`，并在同名记录中注明硬件、固件提交、条件和结论。仓库默认忽略这些本地原始数据，避免误提交设备信息或大文件。

STM32 点灯/UART 任务开始前可检查本机工具链：

```bash
python3 tools/check_stm32_toolchain.py
```

默认输出只包含 `FOUND`/`MISSING`，避免把个人机器绝对路径写入项目记录。

## Issue #3 UART 心跳验收

点灯固件烧录并完成单向 UART 接线后，使用本机实际串口设备运行：

```bash
python3 tools/check_bringup_uart.py \
  --port <serial-port> \
  --required-heartbeats 3 \
  --output data/raw/<new-bringup-record>.txt
```

工具固定按 115200 8N1 接收，并只接受
`rpm_sync_bringup,v1,board=weact_g431_qfn48,mode=MONITOR_ONLY`。读取有总时限，原始记录使用独占创建模式，已有文件不会被覆盖。串口设备名只在本地命令行传入，不写进仓库记录。
