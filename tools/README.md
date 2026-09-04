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

## Issue #8 PPR 标定计算

成员 A 在拆桨、可靠固定并完成供电/急停复核后采集数据；成员 B 使用
`calibrate_ppr.py` 计算每个电机的整数 PPR 候选和参考转速误差。输入 CSV 为：

```csv
motor_id,point_id,reference_rpm,hall_period_us
left,low,<reference-rpm>,<hall-period-us>
left,high,<reference-rpm>,<hall-period-us>
```

每个电机至少需要两个不同的稳定转速点。处理结果必须写入新的
`data/processed/` 文件，工具用独占创建模式防止覆盖：

```bash
python3 tools/calibrate_ppr.py data/raw/<new-calibration>.csv \
  --output data/processed/<new-calibration>-ppr.csv
```

如果没有可追溯的转速计精度资料，摘要固定为
`UNVERIFIED_REFERENCE_ACCURACY`，即使观测误差小于 3% 也不能宣称通过。
只有取得说明书、证书或规格来源后才同时传入以下两个参数：

```bash
--reference-accuracy-percent <percent> --reference-source <evidence-reference>
```

`PASS_CONSERVATIVE_BOUND` 使用保守边界
`最大观测误差 + 参考仪器精度 <= 目标误差`。工具输出的 PPR 始终只是候选，仍需
结合原始脉冲、安装方式、每台磁体数量和复核记录才能写入固件配置。

真实实验日志应写入 `data/raw/`，并在同名记录中注明硬件、固件提交、条件和结论。仓库默认忽略这些本地原始数据，避免误提交设备信息或大文件；例外：2026-09-02 的 L1 霍尔实测原始证据（sigrok `.sr`/`.pvs` 与 PNG，共约 574KB，无设备信息）按 `.gitignore` 显式例外纳入版本控制，便于团队成员直接复核。

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

### Ubuntu 上 CH340 被 brltty 抢占

若 `lsusb` 能看到 `1a86:7523`，但 `/dev/ttyUSB*` 设备节点出现后立即消失，并且内核日志包含 `interface 0 claimed by ch341 while 'brltty' sets config #1`，说明盲文终端服务错误抢占了 CH340，而不是 UART 接线或固件已经验证失败。

本次启动可临时执行：

```bash
sudo systemctl mask --runtime brltty-udev.service
sudo systemctl stop brltty-udev.service
```

随后必须拔下并重新插入 CH340，再确认：

```bash
systemctl is-enabled brltty-udev.service  # 预期 masked-runtime
systemctl is-active brltty-udev.service   # 预期 inactive
ls -l /dev/ttyUSB*
```

`--runtime` 只在本次开机期间有效，重启后恢复。是否永久禁用或卸载 `brltty` 取决于主机是否需要盲文设备支持，不作为项目脚本自动执行。
