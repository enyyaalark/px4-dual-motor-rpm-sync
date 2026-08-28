import importlib.util
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).parents[1] / "tools" / "plot_rpm_log.py"
SPEC = importlib.util.spec_from_file_location("plot_rpm_log", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
plot_rpm_log = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(plot_rpm_log)


class PlotRpmLogTests(unittest.TestCase):
    def test_loads_synthetic_demo(self):
        path = Path(__file__).parents[1] / "data" / "demo" / "synthetic_rpm_log.csv"
        rows = plot_rpm_log.load_csv(path)
        self.assertEqual(11, len(rows))
        self.assertEqual("INIT", rows[0]["system_state"])
        self.assertEqual(7350.0, rows[4]["rpm1"])

    def test_rejects_missing_required_columns(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.csv"
            path.write_text("timestamp_ms,rpm1\n0,0\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "missing required fields"):
                plot_rpm_log.load_csv(path)


if __name__ == "__main__":
    unittest.main()
