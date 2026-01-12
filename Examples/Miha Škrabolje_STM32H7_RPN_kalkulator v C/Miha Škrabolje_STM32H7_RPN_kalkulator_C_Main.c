/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "eth.h"
#include "fdcan.h"
#include "ltdc.h"
#include "quadspi.h"
#include "rtc.h"
#include "sai.h"
#include "sdmmc.h"
#include "spi.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"
#include "fmc.h"

#include <stdlib.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

#define MAX_STACK_SIZE 100   // Maksimalna velikost sklada

#define BUFSIZE 256

struct Stack {
    char **stack;  // Polje za shranjevanje znakov
    int stackPointer; // Kazalec na vrh sklada
};

// Funkcija za inicializacijo skladne strukture
void initializeStack(struct Stack *s) {
	s->stack = (char**) malloc(MAX_STACK_SIZE*sizeof(char*));
    s->stackPointer = -1;  // Nastavi kazalec na prazen sklad

    for (int i=0; i<MAX_STACK_SIZE; i++)
    	s->stack[i] = (char*) malloc(MAX_STACK_SIZE*sizeof(char));
}

void destroyStack(struct Stack *s) {
	for (int i=0; i<MAX_STACK_SIZE; i++)
		free(s->stack[i]);

	free(s->stack);
}


void push(struct Stack *s, char *str) {
    if (s->stackPointer < MAX_STACK_SIZE - 1) {
    	s->stackPointer++;
        strcpy(s->stack[s->stackPointer],str);
    } else {
        printf("Sklad je poln. Znak %s ni bil dodan.\n", str);
    }
}

char* pop(struct Stack *s) {
    if (s->stackPointer >= 0) {
    	int stari = s->stackPointer;
    	s->stackPointer = s->stackPointer - 1;
        return s->stack[stari];
    } else {
        printf("Sklad je prazen. Vrnjen bo NULL znak.\n");
        return '\0';
    }
}

int top(struct Stack *s)
{
	return s->stackPointer;
}

char* topElement(struct Stack *s)
{
	return s->stack[top(s)];
}

void izracun(struct Stack *s)
{
	char *operacija = pop(s);
	int drugo = atoi(pop(s));
	int prvo = atoi(pop(s));

	char *rez = (char*) malloc(MAX_STACK_SIZE*sizeof(char));
	int rezultat = 0;

	if (strcmp(operacija,"+") == 0)
	{
		rezultat = prvo + drugo;
		sprintf(rez, "%d", rezultat);
		push(s,rez);
	}
	else if (strcmp(operacija,"-") == 0)
	{
		rezultat = prvo - drugo;
		sprintf(rez, "%d", rezultat);
		push(s,rez);
	}
	else if (strcmp(operacija,"*") == 0)
	{
		rezultat = prvo * drugo;
		sprintf(rez, "%d", rezultat);
		push(s,rez);
	}
	else if (strcmp(operacija,"/") == 0)
	{
		rezultat = prvo / drugo;
		sprintf(rez, "%d", rezultat);
		push(s,rez);
	}
	else if (strcmp(operacija,"%") == 0)
	{
		rezultat = prvo % drugo;
		sprintf(rez, "%d", rezultat);
		push(s,rez);
	}

	free(rez);
}

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
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

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_ADC3_Init();
  MX_ETH_Init();
  MX_FDCAN1_Init();
  MX_FDCAN2_Init();
  MX_FMC_Init();
  MX_LTDC_Init();
  MX_QUADSPI_Init();
  MX_RTC_Init();
  MX_SAI2_Init();
//  MX_SDMMC1_MMC_Init();
  MX_SPI2_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  /* USER CODE BEGIN 2 */

  // USB_OTG_FS_PCD, USART3, SDMMC1, RTC, LTDC, FMC don't work from RAM !!!!
  // SDMMC1 doesn't work from FLASH !!!!

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  char izraz[BUFSIZE];
  char vpisanZnak;

  strcpy(izraz,"");
  int dolzinaIzraza = 0;

  char *zacetnoBesedilo = {"Kalkulator obrnjenega poljskega zapisa"};
  char *napaka = {"Neveljaven vnos - vneses lahko samo stevilke, presledke, +, -, *, /, % in tipko enter za izračun"};
  char *napakaIzraz = {"Sklad vsebuje več kot en element, zato izraz, ki je bil vpisan ni bil pravilen - poskusite ponovno"};
  char *meja = {"-------------"};

  HAL_UART_Transmit(&huart3,zacetnoBesedilo,strlen(zacetnoBesedilo),1000);
  HAL_UART_Transmit(&huart3,"\r\n",2,10);
  HAL_UART_Transmit(&huart3,meja,strlen(meja),100);
  HAL_UART_Transmit(&huart3,"\r\n",2,10);

  while (1)
  {
	  	HAL_UART_Receive(&huart3,&vpisanZnak,1,1);
	    HAL_UART_Transmit(&huart3,&vpisanZnak,1,1);

	    if ((vpisanZnak >= '0' && vpisanZnak <= '9') || vpisanZnak == ' ' || vpisanZnak == '+' || vpisanZnak == '-' || vpisanZnak == '*' || vpisanZnak == '/' || vpisanZnak == '%')
	    {
	    	strncat(izraz, &vpisanZnak, 1);
	    	dolzinaIzraza++;
	    }
	    else if (vpisanZnak == '\r' && strlen(izraz) > 0)
	    {
	    	char *trenuten = (char*) malloc(MAX_STACK_SIZE*sizeof(char));

	    	char prejsnjiZnak = ' ';

	    	struct Stack sklad;
	    	initializeStack(&sklad);

	    	for (int i=0; i<dolzinaIzraza; i++)
	    	{
	    	  	if (prejsnjiZnak != ' ' && izraz[i] == ' ')
	    	  	{
	    	  		push(&sklad,trenuten);
	    	  		trenuten[0] = '\0';
	    	  		prejsnjiZnak = ' ';
	    	  	}
	    	  	if (izraz[i] == '+' || izraz[i] == '-' || izraz[i] == '*' || izraz[i] == '/' || izraz[i] == '%')
	    	  	{
	    	  		strncat(trenuten, &(izraz[i]), 1);
	    	  		push(&sklad,trenuten);
	    	  		trenuten[0] = '\0';
	    	  		izracun(&sklad);
	    	  	}
	    	  	if (izraz[i] >= '0' && izraz[i] <= '9')
	    	  	{
	    	  		strncat(trenuten, &(izraz[i]), 1);
	    	  		prejsnjiZnak = izraz[i];
	    	  	}
	    	}

	    	if (top(&sklad) == 0)
	    	{
	    		char *rezultat = (char*) malloc(MAX_STACK_SIZE*sizeof(char));
	    		strcpy(rezultat,topElement(&sklad));

	    		HAL_UART_Transmit(&huart3,"\r\n",2,10);
	    		HAL_UART_Transmit(&huart3,rezultat,sizeof(rezultat),100);
	    		HAL_UART_Transmit(&huart3,"\r\n",2,10);

	    		free(rezultat);
	    	}
	    	else
	    	{
	    		HAL_UART_Transmit(&huart3,"\r\n",2,10);
	    		HAL_UART_Transmit(&huart3,napakaIzraz,strlen(napakaIzraz),1000);
	    		HAL_UART_Transmit(&huart3,"\r\n",2,10);
	    	}

	    	HAL_UART_Transmit(&huart3,meja,strlen(meja),100);
	    	HAL_UART_Transmit(&huart3,"\r\n",2,10);

	    	free(trenuten);

	    	destroyStack(&sklad);

	    	strcpy(izraz,"");
	    	dolzinaIzraza = 0;
	    }
	    else if (vpisanZnak != '\0')
	    {
	        HAL_UART_Transmit(&huart3,"\r\n",2,10);
	    	HAL_UART_Transmit(&huart3,napaka,strlen(napaka),1000);
	    	HAL_UART_Transmit(&huart3,"\r\n",2,10);
	    	HAL_UART_Transmit(&huart3,meja,strlen(meja),100);
	    	HAL_UART_Transmit(&huart3,"\r\n",2,10);
	    }

	    vpisanZnak = '\0';

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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 22;
  RCC_OscInitStruct.PLL.PLLN = 169;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_0;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.PLL2.PLL2M = 2;
  PeriphClkInitStruct.PLL2.PLL2N = 12;
  PeriphClkInitStruct.PLL2.PLL2P = 5;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOMEDIUM;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
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
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
