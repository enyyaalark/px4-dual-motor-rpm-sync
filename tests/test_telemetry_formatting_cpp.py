import csv
import io
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
APP = ROOT / "firmware" / "stm32" / "App"
CPP_TEST = ROOT / "tests" / "cpp" / "test_telemetry_formatting.cpp"
EXPECTED_FIELDS = (
    "timestamp_ms",
    "base_pwm_us",
    "rpm1",
    "rpm2",
    "error_rpm",
    "error_percent",
    "correction_us",
    "pwm1_us",
    "pwm2_us",
    "system_state",
    "fault_flags",
)


class TelemetryFormattingCppTests(unittest.TestCase):
    def test_cpp_output_is_valid_csv(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            executable = Path(temporary_directory) / "test_telemetry_formatting"
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
                    str(APP / "telemetry_uart.cpp"),
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
                "Telemetry formatting host logic tests passed",
                test_result.stdout,
            )

            csv_block = test_result.stdout.split("CSV_BEGIN\n", 1)[1].split(
                "CSV_END\n", 1
            )[0]
            reader = csv.DictReader(io.StringIO(csv_block))
            self.assertEqual(list(EXPECTED_FIELDS), reader.fieldnames)
            rows = list(reader)
            self.assertEqual(1, len(rows))
            for field in EXPECTED_FIELDS[:9]:
                float(rows[0][field])
            self.assertEqual("MONITOR_ONLY", rows[0]["system_state"])
            self.assertEqual("0x0003", rows[0]["fault_flags"])


if __name__ == "__main__":
    unittest.main()
