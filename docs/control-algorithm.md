# 控制算法

## PWM 输入有效性

输入捕获/中断层只记录最新原始脉宽、更新时间和“已有样本”标志。范围判断和超时判断在周期任务使用一致的数据快照完成，不在捕获路径写死尚未测量的 PX4 输入参数。

`pwm_input` 的纯逻辑接口由调用方提供最小脉宽、最大脉宽和超时。未确认参数在 `app_config.hpp` 中保持零值，使配置校验失败；只有测量 PX4 实际输出并评审接收容差后才可写入非零值。状态包括：

- `kWaitingForSample`：尚未捕获输入，有效脉宽为 0；
- `kValid`：样本未超时且位于调用方提供的闭区间内；
- `kTimedOut`：样本已过期，有效脉宽为 0；
- `kOutOfRange`：原始脉宽越界，有效脉宽为 0，但保留原始值供诊断；
- `kInvalidConfig`：最小值为 0、最小值大于最大值或超时为 0，拒绝输出。

毫秒时间使用无符号 32 位差值，可处理两次检查间最多一次回绕。主机测试中的范围和超时只是 synthetic 测试向量，不能证明 PX4 输出协议、实际范围、捕获精度或信号电平。

## RPM 计算

```text
RPM = 60 * pulse_frequency_hz / pulses_per_revolution
```

`pulses_per_revolution` 必须用激光转速计或其他独立方法标定，不根据磁极数量猜测。停止超时后 RPM 必须归零。

建议同时保留原始周期、瞬时 RPM 和经明确时间常数滤波后的控制 RPM。滤波不能掩盖脉冲丢失。

### 硬件无关计算状态

`rpm_capture` 用无符号 32 位减法计算相邻捕获值的差，可处理相邻脉冲之间最多一次计时器回绕。首个脉冲只建立基准，不产生周期或 RPM；脉冲时间戳是否存在由独立布尔状态表示，因此系统启动后 `now_ms == 0` 仍是合法时间。

`hall_monitor` 的纯逻辑接口由调用方传入计时器频率、PPR、停止超时和最大合理 RPM。上述参数尚未标定时保持 `TBD`/零值；零值、负值、NaN 或无穷值属于无效配置，不能输出有效 RPM。计算结果分为：

- `kWaitingForPeriod`：尚无两个脉冲，有效 RPM 为 0；
- `kValid`：原始 RPM 在调用方提供的合理范围内，可用 RPM 等于原始 RPM；
- `kTimedOut`：最后脉冲超过调用方提供的超时，有效 RPM 和原始 RPM 均为 0；
- `kInvalidConfig`：PPR 等配置无效，拒绝输出；
- `kImplausiblePulse`：周期为零、计算失败或超过调用方提供的最大 RPM，有效 RPM 为 0；若公式可计算则保留 `raw_rpm` 供诊断。

这些状态只完成纯计算和异常隔离，尚未接入 HAL、中断、故障标志或控制状态机。最大 RPM、脉冲跳变阈值和停止超时必须根据实测波形及转速范围确定，主机合成测试不能代替该标定。

## 误差

第一阶段仅在 `base_pwm1 == base_pwm2`（允许小容差）时验证同步：

```text
error_rpm = rpm1 - rpm2
mean_rpm = (rpm1 + rpm2) / 2
error_percent = abs(error_rpm) / mean_rpm * 100
```

`mean_rpm` 过低时不计算百分比并禁止闭环，防止除零和低速噪声放大。

## P/PI 控制

```text
if abs(error_rpm) <= deadband_rpm:
    effective_error = 0
else:
    effective_error = error_rpm

integral = clamp(integral + effective_error * dt, integral_min, integral_max)
correction = clamp(Kp * effective_error + Ki * integral,
                   -correction_limit_us, correction_limit_us)

pwm1 = clamp(base_pwm - correction, pwm_min_us, pwm_max_us)
pwm2 = clamp(base_pwm + correction, pwm_min_us, pwm_max_us)
```

P 控制先行；只有开环基线和 P 控制数据表明存在稳定残差且无振荡时才启用 `Ki`。

## 必需保护

- 修正量限制：初值不超过基础指令跨度约 5%，最终依数据调整。
- 最终 PWM 限幅：暂按 1000–2000µs，必须用实际电调校准替换。
- RPM 死区和低速闭环禁止。
- 积分限幅；输出饱和或故障时冻结/回退积分。
- 霍尔脉冲超时、非物理跳变和双路不一致检测。
- PX4 PWM 丢失、越界或两个基础目标不一致时停止“等转速”同步。
- 任一电机疑似失效时不得通过不断抬高另一侧/故障侧输出追赶。
- 故障时清零修正，转入 `BYPASS` 或经评审的安全状态。

## 参数调整顺序

1. `Ki = 0`、修正限幅很小，记录开环数据。
2. 从低 `Kp` 开始阶梯增加，每次只改一个参数。
3. 检查稳态误差、1–2秒恢复目标、振荡、修正饱和和温升。
4. 只有 P 控制结果可重复后评估 PI。
5. 每次参数变更记录固件提交、测试条件和 CSV。

## 差动推力兼容

未来若 PX4 给出不同基础指令，控制目标必须保留上层要求的目标差值；第一阶段直接退出同步控制，而不是强制两侧 RPM 相等。
