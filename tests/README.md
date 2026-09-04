# 测试

当前测试覆盖主机侧 CSV 读取/校验、RPM 与 PWM 输入纯逻辑 synthetic 用例和固件 C++17 文件语法，不代表 STM32、PX4、霍尔、电调或闭环控制已验证。

```bash
python3 -m unittest discover -s tests -v
python3 tools/plot_rpm_log.py data/demo/synthetic_rpm_log.csv --validate-only
c++ -std=c++17 -Wall -Wextra -Werror -fsyntax-only firmware/stm32/App/*.cpp
```

`test_pwm_input_logic_cpp.py` 会临时编译并执行 `tests/cpp/test_pwm_input_logic.cpp`，直接验证应用层 C++17 实现。用例中的脉宽范围和超时均为 synthetic 参数，只覆盖状态和边界行为，不是 PX4 输入配置或实机测量结论。

`test_rpm_logic_cpp.py` 会临时编译并执行 `tests/cpp/test_rpm_logic.cpp`，直接验证应用层 C++17 实现。用例使用明确的 synthetic 参数，覆盖首脉冲、正常公式、计时器/毫秒计时回绕、停止超时、无效 PPR 和异常周期；这些数值不是硬件配置或标定结论。

`test_system_controller.py` 会临时编译并执行 `tests/cpp/test_system_controller.cpp`，直接验证 `system_controller` 的状态迁移、旁路门控、基础 PWM 透传、同步修正、低速禁止，以及霍尔超时/异常脉冲和 PWM 超时/越界的故障刷新、修正与积分清零和当前非锁存恢复行为。用例只使用 synthetic 输入，未接入 HAL、定时器或 UART；锁存和去抖策略仍为 `TBD`。

硬件测试证据按 `docs/test-plan.md` 保存，并在对应 Issue/PR 中链接。

`test_calibrate_ppr.py` 覆盖双电机整数 PPR 候选、保守误差边界、稳定点数量、无效
输入和处理结果不可覆盖。用例完全是 synthetic 数值，不是实机 PPR 或精度证据。

`test_pwm_output_adapter_cpp.py` 从 C ABI 调用真实 C++17 双路输出校验，覆盖成对限幅、
非有限请求整组隔离、空指针和目标默认配置保持禁用。它不配置或启动 TIM1，也不证明
真实 PWM 频率、脉宽、电平、同步更新或复位瞬态。
