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

    def test_dual_px4_pwm_inputs_use_independent_one_megahertz_timers(self):
        ioc = load_ioc()

        self.assertEqual("S_TIM3_CH1", ioc["PA6.Signal"])
        self.assertEqual("PX4_PWM1_CAPTURE", ioc["PA6.GPIO_Label"])
        self.assertEqual("S_TIM4_CH1", ioc["PB6.Signal"])
        self.assertEqual("PX4_PWM2_CAPTURE", ioc["PB6.GPIO_Label"])
        for timer in ("TIM3", "TIM4"):
            self.assertEqual("15", ioc[f"{timer}.Prescaler"])
            self.assertEqual("65535", ioc[f"{timer}.Period"])
            self.assertEqual("TIM_SLAVEMODE_RESET", ioc[f"{timer}.SlaveMode"])
            self.assertEqual("TIM_TS_TI1FP1", ioc[f"{timer}.TriggerSource"])
            self.assertEqual(
                "TIM_INPUTCHANNELPOLARITY_RISING",
                ioc[f"{timer}.ICPolarity_CH1"],
            )
            self.assertEqual(
                "TIM_INPUTCHANNELPOLARITY_FALLING",
                ioc[f"{timer}.ICPolarity_CH2"],
            )
            self.assertEqual("0", ioc[f"{timer}.ICFilter_CH1"])
            self.assertEqual("0", ioc[f"{timer}.ICFilter_CH2"])
            self.assertIn("true", ioc[f"NVIC.{timer}_IRQn"])

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

    def test_pwm_capture_interrupt_is_bounded_and_telemetry_is_main_loop_only(self):
        capture_source = (STM32 / "Src" / "pwm_input_capture.c").read_text()
        main_source = (STM32 / "Src" / "main.c").read_text()

        self.assertIn("HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_1)", capture_source)
        self.assertIn("HAL_TIM_IC_Start(htim, TIM_CHANNEL_2)", capture_source)
        self.assertNotIn("HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_2)", capture_source)
        self.assertIn("HAL_TIM_ReadCapturedValue", capture_source)
        self.assertNotIn("HAL_UART_Transmit", capture_source)
        self.assertNotIn("while (", capture_source)
        self.assertIn("PwmInputCapture_Read(pwm_input_snapshots)", main_source)
        self.assertIn("PwmInputEvaluator_EvaluateConfigured", main_source)
        self.assertIn("rpm_sync_pwm_input,v1", main_source)

    def test_cpp_rpm_adapter_is_linked_into_target_project(self):
        project = (STM32 / "STM32CubeIDE" / ".project").read_text()

        self.assertIn("org.eclipse.cdt.core.ccnature", project)
        self.assertIn("Application/User/rpm_evaluator.cpp", project)
        self.assertIn("Application/User/rpm_capture.cpp", project)
        self.assertIn("Application/User/hall_monitor.cpp", project)

    def test_cpp_pwm_input_adapter_is_linked_into_target_project(self):
        project = (STM32 / "STM32CubeIDE" / ".project").read_text()

        self.assertIn("Application/User/pwm_input_capture.c", project)
        self.assertIn("Application/User/pwm_input_evaluator.cpp", project)
        self.assertIn("Application/User/pwm_input.cpp", project)

    def test_monitor_only_pwm_input_configuration_is_explicit(self):
        config = (STM32 / "App" / "app_config.hpp").read_text()

        self.assertIn("kPwmInputMinUs = 950U", config)
        self.assertIn("kPwmInputMaxUs = 1'950U", config)
        self.assertIn("kPwmInputTimeoutMs = 10U", config)
        self.assertIn("kSyncControlDefaultOn = false", config)

    def test_cpp_pwm_output_adapter_is_linked_but_timer_is_not_guessed(self):
        project = (STM32 / "STM32CubeIDE" / ".project").read_text()
        ioc = load_ioc()

        self.assertIn("Application/User/pwm_output_adapter.cpp", project)
        self.assertIn("Application/User/pwm_output.cpp", project)
        self.assertNotIn("TIM1", ioc.get("Mcu.IP0", ""))
        self.assertNotIn("S_TIM1", "\n".join(f"{key}={value}" for key, value in ioc.items()))

    def test_team_accepted_monitor_only_rpm_configuration_stays_closed_loop_off(self):
        config = (STM32 / "App" / "app_config.hpp").read_text()

        self.assertIn("kPulsesPerRevolution = 1.0F", config)
        self.assertIn("kMaximumRpm = 3'300.0F", config)
        self.assertIn("kHallTimeoutMs = 100U", config)
        self.assertIn("kSyncControlDefaultOn = false", config)


if __name__ == "__main__":
    unittest.main()
