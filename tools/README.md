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
