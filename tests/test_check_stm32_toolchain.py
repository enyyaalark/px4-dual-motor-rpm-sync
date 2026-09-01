import importlib.util
import tempfile
import unittest
from pathlib import Path
from unittest import mock


MODULE_PATH = Path(__file__).parents[1] / "tools" / "check_stm32_toolchain.py"
SPEC = importlib.util.spec_from_file_location("check_stm32_toolchain", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
check_stm32_toolchain = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check_stm32_toolchain)


class CheckStm32ToolchainTests(unittest.TestCase):
    def test_discovers_cubemx_in_standard_user_location(self):
        with tempfile.TemporaryDirectory() as directory:
            home = Path(directory)
            executable = (
                home
                / "STMicroelectronics"
                / "STM32Cube"
                / "STM32CubeMX"
                / "STM32CubeMX"
            )
            executable.parent.mkdir(parents=True)
            executable.touch(mode=0o755)

            with mock.patch.object(check_stm32_toolchain.shutil, "which", return_value=None):
                with mock.patch.object(Path, "glob", return_value=[]):
                    tools = check_stm32_toolchain.discover_tools(home=home)

            self.assertEqual(executable, tools["STM32CubeMX"])

    def test_prefers_tools_available_on_path(self):
        expected = Path("/test/bin/arm-none-eabi-gcc")

        def fake_which(name):
            return str(expected) if name == "arm-none-eabi-gcc" else None

        with mock.patch.object(check_stm32_toolchain.shutil, "which", side_effect=fake_which):
            with mock.patch.object(Path, "glob", return_value=[]):
                tools = check_stm32_toolchain.discover_tools(home=Path("/unused"))

        self.assertEqual(expected, tools["arm-none-eabi-gcc"])


if __name__ == "__main__":
    unittest.main()
