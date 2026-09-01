import re
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
STM32 = ROOT / "firmware" / "stm32"


def load_ioc() -> dict[str, str]:
    values: dict[str, str] = {}
    for raw_line in (STM32 / "rpm_sync_bringup.ioc").read_text().splitlines():
        if "=" in raw_line and not raw_line.startswith("#"):
            key, value = raw_line.split("=", 1)
            values[key] = value
    return values


class Stm32BringupConfigTests(unittest.TestCase):
    def test_board_led_swd_and_uart_mapping(self):
        ioc = load_ioc()

        self.assertEqual("STM32G431CBU6", ioc["Mcu.CPN"])
        self.assertEqual("UFQFPN48", ioc["Mcu.Package"])
        self.assertEqual("GPIO_Output", ioc["PC6.Signal"])
        self.assertEqual("STATUS_LED", ioc["PC6.GPIO_Label"])
        self.assertEqual("SYS_JTMS-SWDIO", ioc["PA13.Signal"])
        self.assertEqual("SYS_JTCK-SWCLK", ioc["PA14.Signal"])
        self.assertEqual("USART1_TX", ioc["PA9.Signal"])
        self.assertEqual("TELEMETRY_TX", ioc["PA9.GPIO_Label"])
        self.assertEqual("115200", ioc["USART1.BaudRate"])

    def test_bringup_scope_excludes_motor_control_peripherals(self):
        ioc = load_ioc()
        enabled_ips = {
            ioc[f"Mcu.IP{index}"] for index in range(int(ioc["Mcu.IPNb"]))
        }

        self.assertEqual({"NVIC", "RCC", "SYS", "USART1"}, enabled_ips)

    def test_uart_path_is_monitor_only_and_does_not_receive(self):
        main_source = (STM32 / "Src" / "main.c").read_text()
        application_sources = "\n".join(
            source.read_text() for source in sorted((STM32 / "Src").glob("*.c"))
        )

        self.assertIn("mode=MONITOR_ONLY", main_source)
        self.assertIn("TELEMETRY_TIMEOUT_MS 20U", main_source)
        self.assertNotIn("HAL_UART_Receive", application_sources)
        self.assertNotIn("HAL_UARTEx_Receive", application_sources)
        self.assertNotIn("HAL_Delay", main_source)

    def test_both_configurations_convert_elf_to_binary(self):
        cproject = (STM32 / "STM32CubeIDE" / ".cproject").read_text()
        configuration_names = re.findall(r'<configuration [^>]*name="(\w+)"', cproject)
        enabled = re.findall(
            r'superClass="com\.st\.stm32cube\.ide\.mcu\.gnu\.managedbuild'
            r'\.option\.convertbinary" value="(\w+)"',
            cproject,
        )

        self.assertEqual(["Debug", "Release"], sorted(configuration_names))
        self.assertEqual(["true", "true"], enabled)

    def test_project_metadata_has_no_local_absolute_paths(self):
        metadata = [
            STM32 / "rpm_sync_bringup.ioc",
            STM32 / ".mxproject",
            STM32 / "STM32CubeIDE" / ".project",
            STM32 / "STM32CubeIDE" / ".cproject",
        ]

        for path in metadata:
            content = path.read_text()
            self.assertIsNone(
                re.search(r"/(?:home|tmp)/", content),
                msg=f"local absolute path found in {path.relative_to(ROOT)}",
            )


if __name__ == "__main__":
    unittest.main()
