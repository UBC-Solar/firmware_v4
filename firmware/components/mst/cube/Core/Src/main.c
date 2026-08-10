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
#include "adc.h"
#include "can.h"
#include "crc.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mst_defs.h"
#include "mst_main.h"
#include "gpio_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

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
  MX_DMA_Init();
  MX_CAN_Init();
  MX_CRC_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  Initialize();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /**
     * Mainloop
     */
    #if RUN_MAIN_LOOP
    CollectBoardData();
    CollectModuleData();
    AnalyzeModuleData();
    DriveOutputs();
    SendCanMessages();
    #endif // RUN_MAIN_LOOP

    /**
     * Tests
     */
    #if (UNIT_TEST_MCU == RUN)
    Debug_McuTestCycle();
    #endif // (UNIT_TEST_MCU == RUN)

    #if (UNIT_TEST_IO == RUN)
    Debug_DigitalIoTestCycle();
    #endif // (UNIT_TEST_IO == RUN)

    #if (UNIT_TEST_CAN == RUN)
    Debug_CanTestCycle();
    #endif // (UNIT_TEST_CAN == RUN)
    
    #if (UNIT_TEST_ISOSPI == RUN)
    Debug_IsoSpiTestCycle();
    #endif // (UNIT_TEST_ISOSPI == RUN)

    #if (INT_TEST_SLAVE == RUN)
    Debug_SlaveTestCommsCycle();
    #endif // (INT_TEST_SLAVE == RUN)

    #if (INT_TEST_SLAVE_BAL_VOLT == RUN)
    Debug_SlaveTestBalancingVoltageDrop();
    #endif // (INT_TEST_SLAVE_BAL_VOLT == RUN)

    #if (INT_TEST_SLAVE_BAL_SCRUT == RUN)
    Debug_SlaveTestBalanceScrutCycle();
    #endif // (INT_TEST_SLAVE_BAL_SCRUT == RUN)

    #if (INT_TEST_SLAVE_MUX == RUN)
    Debug_SlaveTestMuxCycle();
    #endif // (INT_TEST_SLAVE_MUX == RUN)

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/* USER CODE BEGIN 4 */

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
  HAL_GPIO_WritePin(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_SET);
  
  GPIO_Write(HLIM_DIS_OUT_GPIO_Port, HLIM_DIS_OUT_Pin, GPIO_PIN_SET);
  GPIO_Write(LLIM_DIS_OUT_GPIO_Port, LLIM_DIS_OUT_Pin, GPIO_PIN_SET);
  GPIO_Write(CONTACTOR_DIS_OUT_GPIO_Port, CONTACTOR_DIS_OUT_Pin, GPIO_PIN_SET);

  while (1)
  {
    uint32_t clockFreq = HAL_RCC_GetSysClockFreq();
    // Scale factor between repetitions and clock frequency is related to
    // number of clock cycles needed to execute loop - determined imperically
    uint32_t repetitions = clockFreq / (9U * 8U); // 9 clock cycles per loop, 1s / 125ms = 8
    for (uint32_t i = 0; i < repetitions; i++)
    {
      asm volatile (
        "NOP\n"
      );
    }

    GPIO_Toggle(LED_OUT_GPIO_Port, LED_OUT_Pin);
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
