import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).parents[1] / "tools" / "check_pwm_input_uart.py"
SPEC = importlib.util.spec_from_file_location("check_pwm_input_uart", MODULE_PATH)
check_pwm = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = check_pwm
SPEC.loader.exec_module(check_pwm)


def line(ch1: str, ch2: str) -> str:
    return f"rpm_sync_pwm_input,v1,t_ms=10,{ch1},{ch2}"


class CheckPwmInputUartTests(unittest.TestCase):
    def test_accepts_dual_valid_sample(self):
        sample = check_pwm.parse_pwm_input_line(
            line(
                "ch1_seen=1,ch1_period_us=2500,ch1_raw_us=1300,ch1_us=1300,ch1_age_ms=1,ch1_status=VALID",
                "ch2_seen=1,ch2_period_us=2500,ch2_raw_us=1000,ch2_us=1000,ch2_age_ms=1,ch2_status=VALID",
            )
        )
        self.assertEqual((1300, 1000), sample.pulse_width_us)

    def test_accepts_waiting_and_timeout(self):
        sample = check_pwm.parse_pwm_input_line(
            line(
                "ch1_seen=0,ch1_period_us=0,ch1_raw_us=0,ch1_us=0,ch1_age_ms=10,ch1_status=WAITING",
                "ch2_seen=1,ch2_period_us=2500,ch2_raw_us=1000,ch2_us=0,ch2_age_ms=11,ch2_status=TIMED_OUT",
            )
        )
        self.assertEqual(("WAITING", "TIMED_OUT"), sample.status)

    def test_accepts_zero_width_as_out_of_range_raw_evidence(self):
        sample = check_pwm.parse_pwm_input_line(
            line(
                "ch1_seen=1,ch1_period_us=2500,ch1_raw_us=0,ch1_us=0,ch1_age_ms=1,ch1_status=OUT_OF_RANGE",
                "ch2_seen=1,ch2_period_us=2500,ch2_raw_us=1000,ch2_us=1000,ch2_age_ms=1,ch2_status=VALID",
            )
        )
        self.assertEqual(("OUT_OF_RANGE", "VALID"), sample.status)

    def test_rejects_inconsistent_effective_width(self):
        with self.assertRaisesRegex(ValueError, "non-VALID"):
            check_pwm.parse_pwm_input_line(
                line(
                    "ch1_seen=1,ch1_period_us=2500,ch1_raw_us=900,ch1_us=900,ch1_age_ms=1,ch1_status=OUT_OF_RANGE",
                    "ch2_seen=1,ch2_period_us=2500,ch2_raw_us=1000,ch2_us=1000,ch2_age_ms=1,ch2_status=VALID",
                )
            )

    def test_raw_record_is_immutable(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "capture.txt"
            check_pwm.write_raw_record(output, ["one\r\n"])
            self.assertEqual("one\n", output.read_text())
            with self.assertRaises(FileExistsError):
                check_pwm.write_raw_record(output, ["replacement"])


if __name__ == "__main__":
    unittest.main()
