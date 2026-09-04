import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
APP = ROOT / "firmware" / "stm32" / "App"
INC = ROOT / "firmware" / "stm32" / "Inc"
CPP_TEST = ROOT / "tests" / "cpp" / "test_rpm_evaluator.cpp"


class RpmEvaluatorCppTests(unittest.TestCase):
    def test_c_adapter_executes_real_cpp_logic(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            executable = Path(temporary_directory) / "test_rpm_evaluator"
            compile_result = subprocess.run(
                [
                    "c++",
                    "-std=c++17",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    f"-I{APP}",
                    f"-I{INC}",
                    str(CPP_TEST),
                    str(APP / "rpm_evaluator.cpp"),
                    str(APP / "rpm_capture.cpp"),
                    str(APP / "hall_monitor.cpp"),
                    "-o",
                    str(executable),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(0, compile_result.returncode, compile_result.stderr)

            test_result = subprocess.run(
                [str(executable)], capture_output=True, text=True, check=False
            )
            self.assertEqual(0, test_result.returncode, test_result.stderr)
            self.assertIn("RPM C adapter tests passed", test_result.stdout)


if __name__ == "__main__":
    unittest.main()
