# UART CSV 协议

## 目的

第一阶段使用人可读 CSV 发送到电脑。PX4 不会自动理解该数据；未来接入必须有单独解析模块和版本化协议。

## 表头

```text
timestamp_ms,base_pwm_us,rpm1,rpm2,error_rpm,error_percent,correction_us,pwm1_us,pwm2_us,system_state,fault_flags
```

## 字段

| 字段 | 类型/单位 | 说明 |
|---|---|---|
| `timestamp_ms` | uint32, ms | STM32 启动后的单调时间，需处理回绕 |
| `base_pwm_us` | uint16, µs | 第一阶段共同基础指令 |
| `rpm1`, `rpm2` | float, RPM | 当前有效 RPM；停止超时后为 0 |
| `error_rpm` | float, RPM | `rpm1 - rpm2` |
| `error_percent` | float, % | 低速无效时输出空值或明确哨兵，方案 `TBD` |
| `correction_us` | float, µs | 有符号受限修正量 |
| `pwm1_us`, `pwm2_us` | uint16, µs | 最终命令值 |
| `system_state` | enum text | `INIT/MONITOR_ONLY/SYNC_CONTROL/BYPASS/FAULT` |
| `fault_flags` | hex bitmask | 版本化故障位掩码 |

## 示例

```text
timestamp_ms,base_pwm_us,rpm1,rpm2,error_rpm,error_percent,correction_us,pwm1_us,pwm2_us,system_state,fault_flags
1000,1400,8200.0,8100.0,100.0,1.23,0.0,1400,1400,MONITOR_ONLY,0x0000
```

示例仅说明格式，不是实验结果。

## 传输约束

- 波特率、周期和小数位数在测量 UART 带宽后确定，当前 `TBD`。
- 每行以 `\n` 结束；固件启动或协议变更时输出版本注释行的方案待定。
- UART 发送不得阻塞捕获和控制周期；使用缓冲/DMA前先验证简单实现。
- 日志采集端保存原始文件，不原地修改。
