import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
STM32 = ROOT / "firmware" / "stm32"


def load_ioc() -> dict[str, str]:
    values: dict[str, str] = {}
    for raw_line in (STM32 / "rpm_sync_capture.ioc").read_text().splitlines():
        if "=" in raw_line and not raw_line.startswith("#"):
            key, value = raw_line.split("=", 1)
            values[key] = value
    return values


class Stm32CaptureConfigTests(unittest.TestCase):
    def test_dual_hall_inputs_share_one_megahertz_tim2(self):
        ioc = load_ioc()

        self.assertEqual("S_TIM2_CH1", ioc["PA0.Signal"])
        self.assertEqual("HALL1_CAPTURE", ioc["PA0.GPIO_Label"])
        self.assertEqual("S_TIM2_CH2", ioc["PA1.Signal"])
        self.assertEqual("HALL2_CAPTURE", ioc["PA1.GPIO_Label"])
        self.assertEqual("15", ioc["TIM2.Prescaler"])
        self.assertEqual("4294967295", ioc["TIM2.Period"])
        self.assertEqual("16000000", ioc["RCC.APB1TimFreq_Value"])

    def test_capture_edges_are_direct_unfiltered_rising_edges(self):
        ioc = load_ioc()

        for channel in (1, 2):
            self.assertEqual(
                "TIM_INPUTCHANNELPOLARITY_RISING",
                ioc[f"TIM2.ICPolarity_CH{channel}"],
            )
            self.assertEqual("0", ioc[f"TIM2.ICFilter_CH{channel}"])
        self.assertIn("true", ioc["NVIC.TIM2_IRQn"])

    def test_interrupt_path_is_bounded_and_starts_both_channels(self):
        source = (STM32 / "Src" / "hall_capture.c").read_text()

        self.assertIn("HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_1)", source)
        self.assertIn("HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_2)", source)
        self.assertIn("HAL_TIM_ReadCapturedValue", source)
        self.assertNotIn("HAL_UART_Transmit", source)
        self.assertNotIn("while (", source)

    def test_capture_telemetry_is_emitted_outside_interrupt_path(self):
        main_source = (STM32 / "Src" / "main.c").read_text()
        interrupt_source = (STM32 / "Src" / "hall_capture.c").read_text()

        self.assertIn("ch1_valid=%u", main_source)
        self.assertIn("ch2_valid=%u", main_source)
        self.assertIn("HallCapture_Read(hall_snapshots)", main_source)
        self.assertIn("RpmEvaluator_EvaluateConfigured", main_source)
        self.assertIn("rpm_sync_capture,v2", main_source)
        self.assertNotIn("HAL_UART_Transmit", interrupt_source)

    def test_cpp_rpm_adapter_is_linked_into_target_project(self):
        project = (STM32 / "STM32CubeIDE" / ".project").read_text()

        self.assertIn("org.eclipse.cdt.core.ccnature", project)
        self.assertIn("Application/User/rpm_evaluator.cpp", project)
        self.assertIn("Application/User/rpm_capture.cpp", project)
        self.assertIn("Application/User/hall_monitor.cpp", project)


if __name__ == "__main__":
    unittest.main()
