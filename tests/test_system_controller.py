import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
APP = ROOT / "firmware" / "stm32" / "App"
CPP_TEST = ROOT / "tests" / "cpp" / "test_system_controller.cpp"


class SystemControllerCppTests(unittest.TestCase):
    def test_cpp_host_suite(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            executable = Path(temporary_directory) / "test_system_controller"
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
                    str(APP / "system_controller.cpp"),
                    str(APP / "rpm_capture.cpp"),
                    str(APP / "hall_monitor.cpp"),
                    str(APP / "pwm_input.cpp"),
                    str(APP / "pwm_output.cpp"),
                    str(APP / "sync_controller.cpp"),
                    str(APP / "fault_manager.cpp"),
                    str(APP / "bypass_control.cpp"),
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
            self.assertIn(
                "System controller host logic tests passed",
                test_result.stdout,
            )


if __name__ == "__main__":
    unittest.main()
