import pathlib
import subprocess
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
APP = ROOT / "firmware" / "stm32" / "App"


class SyncControllerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp_dir = tempfile.TemporaryDirectory()
        source = pathlib.Path(cls.temp_dir.name) / "sync_controller_test.cpp"
        source.write_text(
            r'''#include "sync_controller.hpp"
#include <cmath>

using rpm_sync::SyncController;
using rpm_sync::SyncControllerConfig;
using rpm_sync::reset;
using rpm_sync::step;

bool near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.001F;
}

int main() {
    const SyncControllerConfig config{1.0F, 1.0F, 0.0F, 100.0F, 10.0F, 3.0F};
    const SyncControllerConfig deadband_config{1.0F, 1.0F, 5.0F, 100.0F, 10.0F, 3.0F};
    SyncController controller{};

    if (!near(step(controller, deadband_config, 101.0F, 100.0F, 0.1F, true), 0.0F)) return 1;
    reset(controller);
    if (!near(step(controller, config, 100.0F, 100.0F, 0.1F, true), 0.0F)) return 2;
    if (!near(step(controller, config, 200.0F, 100.0F, 1.0F, true), 10.0F)) return 3;
    if (!near(controller.integral, 0.0F)) return 4;
    if (!near(step(controller, config, 101.0F, 100.0F, 1.0F, true), 2.0F)) return 5;
    if (!near(controller.integral, 1.0F)) return 6;
    if (!near(step(controller, config, 100.0F, 200.0F, 1.0F, true), -10.0F)) return 7;
    if (!near(controller.integral, 1.0F)) return 8;
    if (!near(step(controller, config, 99.0F, 100.0F, 1.0F, true), 0.0F)) return 9;
    if (!near(controller.integral, 0.0F)) return 10;
    if (!near(step(controller, config, 200.0F, 100.0F, 0.0F, true), 0.0F)) return 11;
    if (!near(controller.integral, 0.0F)) return 12;
    return 0;
}
'''
        )
        cls.binary = pathlib.Path(cls.temp_dir.name) / "sync_controller_test"
        subprocess.run(
            [
                "c++",
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(APP),
                str(source),
                str(APP / "sync_controller.cpp"),
                "-o",
                str(cls.binary),
            ],
            check=True,
            capture_output=True,
            text=True,
        )

    @classmethod
    def tearDownClass(cls):
        cls.temp_dir.cleanup()

    def test_deadband_limits_and_conditional_integration(self):
        result = subprocess.run([str(self.binary)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
