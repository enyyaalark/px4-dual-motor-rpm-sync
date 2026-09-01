import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
APP = ROOT / "firmware" / "stm32" / "App"
CPP_TEST = ROOT / "tests" / "cpp" / "test_pwm_input_logic.cpp"


class PwmInputLogicCppTests(unittest.TestCase):
    def test_cpp_host_suite(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            executable = Path(temporary_directory) / "test_pwm_input_logic"
            compile_result = subprocess.run(
                [
                    "c++",
                    "-std=c++17",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-I",
                    str(APP),
                    str(CPP_TEST),
                    str(APP / "pwm_input.cpp"),
                    "-o",
                    str(executable),
                ],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(
                0,
                compile_result.returncode,
                msg=compile_result.stdout + compile_result.stderr,
            )

            test_result = subprocess.run(
                [str(executable)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(
                0,
                test_result.returncode,
                msg=test_result.stdout + test_result.stderr,
            )
            self.assertIn("PWM input host logic tests passed", test_result.stdout)


if __name__ == "__main__":
    unittest.main()
