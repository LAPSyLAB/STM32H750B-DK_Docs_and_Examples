/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "modbus_crc.h"


/* Private variables ---------------------------------------------------------*/

extern UART_HandleTypeDef huart1;


#define BUFSIZE 64
#define MATRIX_ROWS 12
#define MATRIX_COLS 16

uint32_t matrix[MATRIX_ROWS][MATRIX_COLS];

uint8_t rxData[2 * BUFSIZE + 5]; // Receive buffer for UART
uint8_t txData[16];
int n = 0;
int rxReady = 1;

int rxSize = 2 * BUFSIZE + 5;	//each register takes 2 bytes and 5 are for modbus protocol

int rst = 0;


/* Private function prototypes -----------------------------------------------*/
void sendData (uint8_t *data);


/**
  * @brief  The application entry point.
  * @retval int
  */
int start_uart(void)
{

  __enable_irq();


  while (1)
  {

    /* USER CODE END WHILE */
	  for (int tretjina = 0; tretjina < 3; tretjina++) {
		  if (rst == 1) {

			  tretjina = -1;
			  rst = 0;
			  n = 0;

		  } else if (rxReady == 1) {
			  HAL_Delay(200);

			  HAL_UARTEx_ReceiveToIdle_IT(&huart1, rxData, rxSize);

			  int addressOffset = tretjina * BUFSIZE;


			  txData[0] = 234;  // slave address
			  txData[1] = 0x04;  // Function code for Read Input Registers

			  txData[2] = 0;
			  txData[3] = 9 + addressOffset;	//Reading start with offset

			  txData[4] = 0;
			  txData[5] = BUFSIZE;	//Number of registers we want to read

			  uint16_t crc = crc16(txData, 6);
			  txData[6] = crc&0xFF;   // CRC LOW
			  txData[7] = (crc>>8)&0xFF;  // CRC HIGH

			  rxReady = 0;
			  n = tretjina;
			  sendData(txData);


		  } else {
			  tretjina--;
		  }

	  }

  }
}


void sendData (uint8_t *data)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);

	if (HAL_UART_Transmit(&huart1, data, 8, 2000) != HAL_OK) {
		Error_Handler();
	}

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{

	if ((Size < rxSize) || (rxData[0] != 234) || (n > 2)){
	    rst = 1;
	  } else {
		    int count = 3;	// first three bytes of response are slave address, function code, Byte count

			for (int i = 0; i < 4; i++) {
				for (int col = MATRIX_COLS - 1; col >= 0; col--) {
					matrix[i + (n * 4)][col] = rxData[count]<<8 | rxData[count + 1];
					count = count + 2;
				}
			}
			if (n == 2) {
				rst = 1;
				displayMatrixOnLCD(&matrix);
			}

	  }
	rxReady = 1;
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
