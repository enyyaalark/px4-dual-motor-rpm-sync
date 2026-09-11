import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
APP = ROOT / "firmware" / "stm32" / "App"
INC = ROOT / "firmware" / "stm32" / "Inc"
CPP_TEST = ROOT / "tests" / "cpp" / "test_pwm_input_evaluator.cpp"


class PwmInputEvaluatorCppTests(unittest.TestCase):
    def test_configured_c_adapter(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "test_pwm_input_evaluator"
            subprocess.run(
                [
                    "c++",
                    "-std=c++17",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    f"-I{APP}",
                    f"-I{INC}",
                    str(CPP_TEST),
                    str(APP / "pwm_input_evaluator.cpp"),
                    str(APP / "pwm_input.cpp"),
                    "-o",
                    str(executable),
                ],
                check=True,
            )
            result = subprocess.run(
                [str(executable)], capture_output=True, text=True, check=False
            )
            self.assertEqual(0, result.returncode, result.stderr)


if __name__ == "__main__":
    unittest.main()
