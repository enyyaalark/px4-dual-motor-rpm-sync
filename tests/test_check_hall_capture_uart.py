import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).parents[1] / "tools" / "check_hall_capture_uart.py"
SPEC = importlib.util.spec_from_file_location("check_hall_capture_uart", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
check_capture = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_capture
SPEC.loader.exec_module(check_capture)


class CheckHallCaptureUartTests(unittest.TestCase):
    def test_accepts_expected_capture_line(self):
        sample = check_capture.parse_capture_line(
            "rpm_sync_capture,v1,t_ms=1200,ch1_valid=1,ch1_period_us=5000,"
            "ch1_age_ms=20,ch2_valid=0,ch2_period_us=0,ch2_age_ms=1200\r\n"
        )

        self.assertEqual(1200, sample.timestamp_ms)
        self.assertEqual((True, False), sample.valid)
        self.assertEqual((5000, 0), sample.period_us)
        self.assertEqual((20, 1200), sample.age_ms)
        self.assertEqual((None, None), sample.rpm)
        self.assertEqual((None, None), sample.status)

    def test_accepts_v2_rpm_status_fields(self):
        sample = check_capture.parse_capture_line(
            "rpm_sync_capture,v2,t_ms=1200,ch1_valid=1,ch1_period_us=5000,"
            "ch1_age_ms=20,ch1_raw_rpm=6000,ch1_rpm=6000,ch1_status=VALID,"
            "ch2_valid=1,ch2_period_us=1000,ch2_age_ms=10,ch2_raw_rpm=30000,"
            "ch2_rpm=0,ch2_status=IMPLAUSIBLE_PULSE"
        )

        self.assertEqual((6000, 30000), sample.raw_rpm)
        self.assertEqual((6000, 0), sample.rpm)
        self.assertEqual(("VALID", "IMPLAUSIBLE_PULSE"), sample.status)

    def test_rejects_wrong_schema_invalid_boolean_and_uint32_overflow(self):
        invalid_lines = (
            "rpm_sync_capture,v2,t_ms=1,ch1_valid=0,ch1_period_us=0,"
            "ch1_age_ms=1,ch2_valid=0,ch2_period_us=0,ch2_age_ms=1",
            "rpm_sync_capture,v1,t_ms=1,ch1_valid=2,ch1_period_us=0,"
            "ch1_age_ms=1,ch2_valid=0,ch2_period_us=0,ch2_age_ms=1",
            "rpm_sync_capture,v1,t_ms=4294967296,ch1_valid=0,ch1_period_us=0,"
            "ch1_age_ms=1,ch2_valid=0,ch2_period_us=0,ch2_age_ms=1",
            "rpm_sync_capture,v2,t_ms=1,ch1_valid=0,ch1_period_us=0,"
            "ch1_age_ms=1,ch1_raw_rpm=0,ch1_rpm=0,ch1_status=UNKNOWN,"
            "ch2_valid=0,ch2_period_us=0,ch2_age_ms=1,ch2_raw_rpm=0,"
            "ch2_rpm=0,ch2_status=WAITING",
        )

        for line in invalid_lines:
            with self.subTest(line=line), self.assertRaises(ValueError):
                check_capture.parse_capture_line(line)

    def test_raw_record_is_created_without_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "capture.txt"
            check_capture.write_raw_record(output, ["line1\r\n", "line2"])

            self.assertEqual("line1\nline2\n", output.read_text())
            with self.assertRaises(FileExistsError):
                check_capture.write_raw_record(output, ["replacement"])


if __name__ == "__main__":
    unittest.main()
