import importlib.util
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).parents[1] / "tools" / "check_bringup_uart.py"
SPEC = importlib.util.spec_from_file_location("check_bringup_uart", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
check_bringup_uart = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check_bringup_uart)


class CheckBringupUartTests(unittest.TestCase):
    def test_accepts_expected_monitor_only_heartbeat(self):
        fields = check_bringup_uart.parse_heartbeat(
            "rpm_sync_bringup,v1,board=weact_g431_qfn48,mode=MONITOR_ONLY\r\n"
        )

        self.assertEqual("weact_g431_qfn48", fields["board"])
        self.assertEqual("MONITOR_ONLY", fields["mode"])

    def test_rejects_wrong_version_board_or_mode(self):
        invalid_lines = (
            "rpm_sync_bringup,v2,board=weact_g431_qfn48,mode=MONITOR_ONLY",
            "rpm_sync_bringup,v1,board=unknown,mode=MONITOR_ONLY",
            "rpm_sync_bringup,v1,board=weact_g431_qfn48,mode=SYNC_CONTROL",
        )

        for line in invalid_lines:
            with self.subTest(line=line):
                with self.assertRaises(ValueError):
                    check_bringup_uart.parse_heartbeat(line)

    def test_raw_record_is_created_without_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "bringup.txt"
            check_bringup_uart.write_raw_record(output, ["line1\r\n", "line2"])

            self.assertEqual("line1\nline2\n", output.read_text())
            with self.assertRaises(FileExistsError):
                check_bringup_uart.write_raw_record(output, ["replacement"])


if __name__ == "__main__":
    unittest.main()
