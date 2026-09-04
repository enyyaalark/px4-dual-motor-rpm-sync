/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "hall_capture.h"
#include "rpm_evaluator.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define STATUS_LED_PERIOD_MS 500U
#define TELEMETRY_PERIOD_MS 1000U
#define TELEMETRY_TIMEOUT_MS 20U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static uint32_t last_led_tick_ms;
static uint32_t last_telemetry_tick_ms;
static HallCaptureSnapshot hall_snapshots[2];
static RpmEvaluationResult rpm_results[2];
static uint8_t capture_telemetry[320];
static const uint8_t telemetry_heartbeat[] =
  "rpm_sync_bringup,v1,board=weact_g431_qfn48,mode=MONITOR_ONLY\r\n";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  last_led_tick_ms = HAL_GetTick();
  last_telemetry_tick_ms = last_led_tick_ms;
  (void)HAL_UART_Transmit(&huart1,
                         (uint8_t *)telemetry_heartbeat,
                         sizeof(telemetry_heartbeat) - 1U,
                         TELEMETRY_TIMEOUT_MS);

  if (HallCapture_Start(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    const uint32_t now_ms = HAL_GetTick();

    if ((now_ms - last_led_tick_ms) >= STATUS_LED_PERIOD_MS)
    {
      last_led_tick_ms = now_ms;
      HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
    }

    if ((now_ms - last_telemetry_tick_ms) >= TELEMETRY_PERIOD_MS)
    {
      last_telemetry_tick_ms = now_ms;
      HallCapture_Read(hall_snapshots);
      for (uint32_t channel = 0U; channel < 2U; ++channel)
      {
        const RpmEvaluationInput input = {
          hall_snapshots[channel].period_ticks,
          hall_snapshots[channel].last_pulse_ms,
          hall_snapshots[channel].has_pulse,
          hall_snapshots[channel].has_period
        };
        rpm_results[channel] = RpmEvaluator_EvaluateConfigured(&input, now_ms);
      }
      const int telemetry_length = snprintf(
        (char *)capture_telemetry,
        sizeof(capture_telemetry),
        "rpm_sync_capture,v2,t_ms=%lu,ch1_valid=%u,ch1_period_us=%lu,"
        "ch1_age_ms=%lu,ch1_raw_rpm=%lu,ch1_rpm=%lu,ch1_status=%s,"
        "ch2_valid=%u,ch2_period_us=%lu,ch2_age_ms=%lu,ch2_raw_rpm=%lu,"
        "ch2_rpm=%lu,ch2_status=%s\r\n",
        (unsigned long)now_ms,
        (unsigned int)hall_snapshots[0].has_period,
        (unsigned long)rpm_results[0].period_ticks,
        (unsigned long)(now_ms - hall_snapshots[0].last_pulse_ms),
        (unsigned long)rpm_results[0].raw_rpm,
        (unsigned long)rpm_results[0].rpm,
        RpmEvaluator_StatusName(rpm_results[0].status),
        (unsigned int)hall_snapshots[1].has_period,
        (unsigned long)rpm_results[1].period_ticks,
        (unsigned long)(now_ms - hall_snapshots[1].last_pulse_ms),
        (unsigned long)rpm_results[1].raw_rpm,
        (unsigned long)rpm_results[1].rpm,
        RpmEvaluator_StatusName(rpm_results[1].status));

      if ((telemetry_length > 0) &&
          ((size_t)telemetry_length < sizeof(capture_telemetry)))
      {
        (void)HAL_UART_Transmit(&huart1,
                               capture_telemetry,
                               (uint16_t)telemetry_length,
                               TELEMETRY_TIMEOUT_MS);
      }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  HallCapture_OnInterrupt(htim);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
