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

## 硬件无关格式化边界

`telemetry_uart` 只向调用方提供的固定缓冲区写入，不分配动态内存。成功结果返回不含结尾空字符的完整行长度；只有状态为 `kOk` 时，调用方才可把该长度交给未来 UART 发送层。

空指针、零容量、未声明的系统状态、任一 NaN/无穷浮点值或缓冲区不足都属于格式化失败。若缓冲区可写，失败会清除首字节，避免把 `snprintf` 产生的截断半行误发为有效 CSV。低速时 `error_percent` 应由上游提供什么有限值仍为 `TBD`，本规则不替代该协议决策。

主机测试会把真实 C++ 格式化结果交给 Python 标准 CSV 解析器，验证 11 字段顺序及前 9 个数值字段；它不验证 UART 波特率、周期、发送队列、DMA、丢行率或实时影响。

当前实现使用 `snprintf` 浮点格式。主机及 GNU Arm 语法检查不能证明目标板链接的 newlib/newlib-nano 已包含浮点格式化支持，也不能证明 Flash、栈和执行时间开销可接受；这些必须在 #14 的目标板构建和周期测量中确认，必要时再改为受控的定点格式化。
