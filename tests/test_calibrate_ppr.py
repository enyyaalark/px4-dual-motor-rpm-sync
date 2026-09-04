import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).parents[1] / "tools" / "calibrate_ppr.py"
SPEC = importlib.util.spec_from_file_location("calibrate_ppr", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
calibrate_ppr = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = calibrate_ppr
SPEC.loader.exec_module(calibrate_ppr)


class CalibratePprTests(unittest.TestCase):
    def measurements(self):
        return [
            calibrate_ppr.Measurement("left", "low", 6000.0, 5000.0),
            calibrate_ppr.Measurement("left", "high", 12000.0, 2500.0),
            calibrate_ppr.Measurement("right", "low", 6000.0, 3333.333333),
            calibrate_ppr.Measurement("right", "high", 12000.0, 1666.666667),
        ]

    def test_estimates_candidate_per_motor(self):
        results, summaries = calibrate_ppr.analyze(self.measurements(), 3.0, None)
        self.assertEqual([2, 2, 3, 3], [result.candidate_ppr for result in results])
        self.assertTrue(all(result.observed_error_percent < 0.001 for result in results))
        self.assertEqual(["UNVERIFIED_REFERENCE_ACCURACY"] * 2,
                         [summary.qualification for summary in summaries])

    def test_conservative_accuracy_qualification(self):
        _, summaries = calibrate_ppr.analyze(self.measurements(), 3.0, 1.0)
        self.assertEqual(["PASS_CONSERVATIVE_BOUND"] * 2,
                         [summary.qualification for summary in summaries])

    def test_reference_uncertainty_can_make_result_inconclusive(self):
        _, summaries = calibrate_ppr.analyze(self.measurements(), 3.0, 4.0)
        self.assertEqual(["INCONCLUSIVE_REFERENCE_ACCURACY"] * 2,
                         [summary.qualification for summary in summaries])

    def test_observed_error_can_fail_target(self):
        rows = [
            calibrate_ppr.Measurement("left", "low", 6000.0, 5000.0),
            calibrate_ppr.Measurement("left", "high", 13000.0, 2500.0),
        ]
        _, summaries = calibrate_ppr.analyze(rows, 3.0, 0.0)
        self.assertEqual("FAIL_OBSERVED_ERROR", summaries[0].qualification)

    def test_requires_multiple_distinct_points(self):
        rows = [
            calibrate_ppr.Measurement("left", "same", 6000.0, 5000.0),
            calibrate_ppr.Measurement("left", "same", 6000.0, 5000.0),
        ]
        _, summaries = calibrate_ppr.analyze(rows, 3.0, 0.5)
        self.assertEqual("INSUFFICIENT_POINTS", summaries[0].qualification)

    def test_load_rejects_non_positive_measurement(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.csv"
            path.write_text("motor_id,point_id,reference_rpm,hall_period_us\nleft,low,0,5000\n",
                            encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "positive and finite"):
                calibrate_ppr.load_measurements(path)

    def test_output_does_not_overwrite_existing_file(self):
        results, _ = calibrate_ppr.analyze(self.measurements(), 3.0, None)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "results.csv"
            calibrate_ppr.write_results(path, results)
            self.assertEqual(5, len(path.read_text(encoding="utf-8").splitlines()))
            with self.assertRaises(FileExistsError):
                calibrate_ppr.write_results(path, results)


if __name__ == "__main__":
    unittest.main()
