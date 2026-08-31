# 测试

当前测试覆盖主机侧 CSV 读取/校验、PWM 输入纯逻辑 synthetic 用例和固件 C++17 文件语法，不代表 STM32、PX4、霍尔、电调或闭环控制已验证。

```bash
python3 -m unittest discover -s tests -v
python3 tools/plot_rpm_log.py data/demo/synthetic_rpm_log.csv --validate-only
c++ -std=c++17 -Wall -Wextra -Werror -fsyntax-only firmware/stm32/App/*.cpp
```

`test_pwm_input_logic_cpp.py` 会临时编译并执行 `tests/cpp/test_pwm_input_logic.cpp`，直接验证应用层 C++17 实现。用例中的脉宽范围和超时均为 synthetic 参数，只覆盖状态和边界行为，不是 PX4 输入配置或实机测量结论。

硬件测试证据按 `docs/test-plan.md` 保存，并在对应 Issue/PR 中链接。
