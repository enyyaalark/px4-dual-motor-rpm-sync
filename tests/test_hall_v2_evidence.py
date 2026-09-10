import importlib.util
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
TOOL_PATH = ROOT / "tools" / "check_hall_capture_uart.py"
SPEC = importlib.util.spec_from_file_location("check_hall_capture_uart", TOOL_PATH)
assert SPEC is not None and SPEC.loader is not None
capture_uart = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = capture_uart
SPEC.loader.exec_module(capture_uart)


class HallV2EvidenceTests(unittest.TestCase):
    raw_dir = ROOT / "data" / "raw" / "2026-09-11"

    def load(self, name: str):
        lines = (self.raw_dir / name).read_text(encoding="utf-8").splitlines()
        self.assertTrue(all(line.startswith("rpm_sync_capture,v2,") for line in lines))
        samples = [capture_uart.parse_capture_line(line) for line in lines]
        self.assertTrue(
            all(
                newer.timestamp_ms - older.timestamp_ms == 1_000
                for older, newer in zip(samples, samples[1:])
            )
        )
        return samples

    def test_run1_has_stable_simultaneous_valid_rpm_without_false_timeout(self):
        samples = self.load("issue-7-dual-stop-v2-run1.txt")

        self.assertEqual(35, len(samples))
        for sample in samples:
            self.assertEqual((True, True), sample.valid)
            self.assertEqual(("VALID", "VALID"), sample.status)
            self.assertTrue(all(rpm is not None and rpm > 0 for rpm in sample.rpm))
            self.assertEqual(sample.raw_rpm, sample.rpm)
            self.assertTrue(all(age <= 100 for age in sample.age_ms))

        self.assertEqual((1721, 1759), (min(s.rpm[0] for s in samples), max(s.rpm[0] for s in samples)))
        self.assertEqual((1587, 1610), (min(s.rpm[1] for s in samples), max(s.rpm[1] for s in samples)))

    def test_run2_transitions_both_channels_to_zero_rpm_timeout(self):
        samples = self.load("issue-7-dual-stop-v2-run2.txt")

        self.assertEqual(18, len(samples))
        self.assertEqual(
            [("VALID", "VALID")] * 7 + [("TIMED_OUT", "TIMED_OUT")] * 11,
            [sample.status for sample in samples],
        )
        for sample in samples[:7]:
            self.assertEqual((True, True), sample.valid)
            self.assertTrue(all(rpm is not None and rpm > 0 for rpm in sample.rpm))
            self.assertEqual(sample.raw_rpm, sample.rpm)
            self.assertTrue(all(age <= 100 for age in sample.age_ms))
        for sample in samples[7:]:
            self.assertEqual((True, True), sample.valid)
            self.assertEqual((0, 0), sample.raw_rpm)
            self.assertEqual((0, 0), sample.rpm)
            self.assertTrue(all(age > 100 for age in sample.age_ms))


if __name__ == "__main__":
    unittest.main()
