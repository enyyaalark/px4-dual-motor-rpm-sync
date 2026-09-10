import csv
import importlib.util
import math
import statistics
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
TOOLS = ROOT / "tools"


def load_tool(name: str):
    path = TOOLS / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


calibrate_ppr = load_tool("calibrate_ppr")
capture_uart = load_tool("check_hall_capture_uart")


class PprCalibrationEvidenceTests(unittest.TestCase):
    sources_path = ROOT / "data" / "processed" / "2026-09-10-ppr-calibration-sources.csv"
    input_path = ROOT / "data" / "processed" / "2026-09-10-ppr-calibration-input.csv"
    results_path = ROOT / "data" / "processed" / "2026-09-10-ppr-calibration-results.csv"

    def load_sources(self):
        with self.sources_path.open(encoding="utf-8", newline="") as stream:
            return list(csv.DictReader(stream))

    def test_six_points_have_fresh_valid_raw_periods_and_matching_medians(self):
        sources = self.load_sources()
        self.assertEqual(6, len(sources))
        self.assertEqual(
            {("motor1", "P1"), ("motor1", "P2"), ("motor1", "P3"),
             ("motor2", "P1"), ("motor2", "P2"), ("motor2", "P3")},
            {(row["motor_id"], row["point_id"]) for row in sources},
        )

        for row in sources:
            channel = int(row["hall_channel"].removeprefix("ch")) - 1
            lines = (ROOT / row["raw_file"]).read_text(encoding="utf-8").splitlines()
            samples = [capture_uart.parse_capture_line(line) for line in lines]
            self.assertEqual(10, len(samples), row["raw_file"])
            self.assertTrue(all(sample.valid[channel] for sample in samples), row["raw_file"])
            self.assertTrue(all(sample.period_us[channel] > 0 for sample in samples), row["raw_file"])
            self.assertTrue(
                all(sample.age_ms[channel] <= math.ceil(sample.period_us[channel] / 1000) + 1
                    for sample in samples),
                row["raw_file"],
            )
            median = statistics.median(sample.period_us[channel] for sample in samples)
            self.assertEqual(float(row["hall_period_median_us"]), median, row["raw_file"])

    def test_committed_input_and_results_reproduce_with_repository_tool(self):
        measurements = calibrate_ppr.load_measurements(self.input_path)
        results, summaries = calibrate_ppr.analyze(measurements, 3.0, None)

        self.assertEqual([1, 1], [summary.candidate_ppr for summary in summaries])
        self.assertEqual(
            ["UNVERIFIED_REFERENCE_ACCURACY", "UNVERIFIED_REFERENCE_ACCURACY"],
            [summary.qualification for summary in summaries],
        )
        with tempfile.TemporaryDirectory() as directory:
            regenerated = Path(directory) / "results.csv"
            calibrate_ppr.write_results(regenerated, results)
            self.assertEqual(
                self.results_path.read_text(encoding="utf-8"),
                regenerated.read_text(encoding="utf-8"),
            )


if __name__ == "__main__":
    unittest.main()
