import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
TOOL_PATH = ROOT / "tools" / "bench_run.py"


def _load_bench_run():
    spec = importlib.util.spec_from_file_location("bench_run", TOOL_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load {TOOL_PATH}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[module.__name__] = module
    spec.loader.exec_module(module)
    return module


bench_run = _load_bench_run()


def _capture_line(timestamp: int, rpm1: int, rpm2: int) -> str:
    return (
        "rpm_sync_capture,v2,t_ms=%d,ch1_valid=1,ch1_period_us=5000,"
        "ch1_age_ms=1,ch1_raw_rpm=%d,ch1_rpm=%d,ch1_status=VALID,"
        "ch2_valid=1,ch2_period_us=5000,ch2_age_ms=1,ch2_raw_rpm=%d,"
        "ch2_rpm=%d,ch2_status=VALID" % (timestamp, rpm1, rpm1, rpm2, rpm2)
    )


def _pwm_line(timestamp: int, ch1_us: int, ch2_us: int) -> str:
    return (
        "rpm_sync_pwm_input,v1,t_ms=%d,ch1_seen=1,ch1_period_us=2500,"
        "ch1_raw_us=%d,ch1_us=%d,ch1_age_ms=1,ch1_status=VALID,"
        "ch2_seen=1,ch2_period_us=2500,ch2_raw_us=%d,ch2_us=%d,"
        "ch2_age_ms=1,ch2_status=VALID" % (timestamp, ch1_us, ch1_us, ch2_us, ch2_us)
    )


def _ctrl_line(
    timestamp: int,
    rpm1: int,
    rpm2: int,
    error_rpm: int,
    error_percent: int,
    select: int,
    state: int,
    pwm1: int = 1060,
    pwm2: int = 1060,
) -> str:
    return (
        "rpm_sync_ctrl,v1,t_ms=%d,base_us=1060,pwm1_us=%d,pwm2_us=%d,"
        "rpm1=%d,rpm2=%d,error_rpm=%d,error_percent=%d,correction_us=0,"
        "select=%d,state=%d,fault=0x0"
        % (timestamp, pwm1, pwm2, rpm1, rpm2, error_rpm, error_percent, select, state)
    )


class BenchRunTests(unittest.TestCase):
    def test_parse_ctrl_line_accepts_expected_order(self):
        line = _ctrl_line(1000, 3000, 3100, -100, 3, 0, 1)
        sample = bench_run.parse_ctrl_line(line)

        self.assertEqual(1000, sample.timestamp_ms)
        self.assertEqual(3000, sample.rpm1)
        self.assertEqual(3100, sample.rpm2)
        self.assertEqual(-100, sample.error_rpm)
        self.assertEqual(0, sample.select)

    def test_parse_ctrl_line_rejects_wrong_field_order(self):
        line = (
            "rpm_sync_ctrl,v1,t_ms=1000,base_us=1060,pwm1_us=1060,pwm2_us=1060,"
            "rpm1=3000,rpm2=3100,error_rpm=-100,error_percent=3,"
            "correction_us=0,state=1,select=0,fault=0x0"
        )
        with self.assertRaises(ValueError):
            bench_run.parse_ctrl_line(line)

    def test_compare_detects_improvement(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            temporary_path = Path(temporary_directory)
            baseline_lines = [
                _capture_line(1, 3000, 3100),
                _pwm_line(1, 1060, 1060),
                _ctrl_line(1000, 3000, 3100, -100, 3, 0, 1),
                _ctrl_line(2000, 3000, 3100, -100, 3, 0, 1),
            ]
            sync_lines = [
                _capture_line(1, 3000, 3030),
                _pwm_line(1, 1060, 1060),
                _ctrl_line(1000, 3000, 3030, -30, 1, 1, 2, 1050, 1070),
                _ctrl_line(2000, 3000, 3030, -30, 1, 1, 2, 1050, 1070),
            ]
            baseline_path = temporary_path / "baseline.txt"
            sync_path = temporary_path / "sync.txt"
            baseline_path.write_text("\n".join(baseline_lines) + "\n", encoding="utf-8")
            sync_path.write_text("\n".join(sync_lines) + "\n", encoding="utf-8")

            baseline_run = bench_run.analyze_file(baseline_path)
            sync_run = bench_run.analyze_file(sync_path)
            baseline_metrics = bench_run.metrics(baseline_run)
            sync_metrics = bench_run.metrics(sync_run)

            self.assertEqual(100.0, baseline_metrics.median_abs_error_rpm)
            self.assertEqual(30.0, sync_metrics.median_abs_error_rpm)
            self.assertEqual(2, sync_metrics.selected_corrected_count)
            self.assertEqual({1}, baseline_metrics.states)
            self.assertEqual({2}, sync_metrics.states)

    def test_capture_rate_excludes_timed_out_v2_status(self):
        timed_out = (
            "rpm_sync_capture,v2,t_ms=2,ch1_valid=1,ch1_period_us=5000,"
            "ch1_age_ms=101,ch1_raw_rpm=0,ch1_rpm=0,ch1_status=TIMED_OUT,"
            "ch2_valid=1,ch2_period_us=5000,ch2_age_ms=1,ch2_raw_rpm=3000,"
            "ch2_rpm=3000,ch2_status=VALID"
        )
        run = bench_run.analyze_lines(
            Path("capture.txt"),
            [_capture_line(1, 3000, 3000), timed_out],
        )

        self.assertEqual(2, run.capture_total)
        self.assertEqual(1, run.capture_both_valid)
        self.assertEqual(0.5, bench_run.metrics(run).capture_both_valid_rate)


if __name__ == "__main__":
    unittest.main()
