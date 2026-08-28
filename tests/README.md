# 测试

当前测试只覆盖主机侧 CSV 读取/校验和固件 C 文件语法，不代表 STM32、PX4、霍尔、电调或闭环控制已验证。

```bash
python3 -m unittest discover -s tests -v
python3 tools/plot_rpm_log.py data/demo/synthetic_rpm_log.csv --validate-only
cc -std=c11 -Wall -Wextra -Werror -fsyntax-only firmware/stm32/App/*.c
```

硬件测试证据按 `docs/test-plan.md` 保存，并在对应 Issue/PR 中链接。
