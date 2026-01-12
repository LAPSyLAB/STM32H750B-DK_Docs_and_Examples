/**
  ******************************************************************************
  * @file    BSP/Src/touchscreen.c
  * @author  MCD Application Team
  * @brief   This example code shows how to use the touchscreen driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup BSP
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define  CIRCLE_RADIUS        40
#define  LINE_LENGHT          30

#define  MATRIX_SIZE_X		  16
#define  MATRIX_SIZE_Y  	  12
/* Private macro -------------------------------------------------------------*/
#define  CIRCLE_XPOS(i)       ((i * 480) / 5)
#define  CIRCLE_YPOS(i)       (272 - CIRCLE_RADIUS - 60)

#define GET_RED(color)   ((color >> 16) & 0xFF)
#define GET_GREEN(color) ((color >> 8) & 0xFF)
#define GET_BLUE(color)  (color & 0xFF)
#define RGB(r, g, b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

uint32_t tempToColor(uint16_t temp);
/* Private Structures and Enumerations ------------------------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
TS_State_t  TS_State;

/* Private variables ---------------------------------------------------------*/
/* Static variable holding the current touch color index : used to change color at each touch */
TS_Init_t hTS;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Touchscreen Demo1 : test touchscreen calibration and single touch in polling mode
  * @param  None
  * @retval None
  */
void Touchscreen_demo(void)
{
  /*uint16_t x1, y1;
  uint8_t state = 0;*/
  uint32_t ts_status = BSP_ERROR_NONE;
  uint32_t x_size, y_size;


  BSP_LCD_GetXSize(0, &x_size);
  BSP_LCD_GetYSize(0, &y_size);

  hTS.Width = x_size;
  hTS.Height = y_size;
  hTS.Orientation =TS_SWAP_XY ;
  hTS.Accuracy = 5;

  /* Touchscreen initialization */
  ts_status = BSP_TS_Init(0, &hTS);

  if(ts_status == BSP_ERROR_NONE)
  {
    /* Display touch screen demo description */
	uint32_t x_size, y_size;

	BSP_LCD_GetXSize(0, &x_size);
	BSP_LCD_GetYSize(0, &y_size);


	/* Clear the LCD */
	UTIL_LCD_Clear(0xFF000000UL);
	start_uart();

    while (1)
    {






      HAL_Delay(100);
    }
  } /* of if(status == BSP_ERROR_NONE) */
}

uint32_t tempToColor(uint16_t temp) {
    // Define color values
    const uint32_t darkBlue = 0xFF000030UL;  // Dark blue color for coldest
    const uint32_t blue = 0xFF02004aUL;    // Blue color for very cold
    const uint32_t lightBlue = 0xFF020098UL;    // Blue color for colder
    const uint32_t lightestBlue = 0xFF5000fdUL;    // Blue color for cold
    const uint32_t purple = 0xFF800080UL;    // Purple color for warm
    const uint32_t yellow = 0xFFFFFF00UL;  // Yellow color for warmer
    const uint32_t orange = 0xFFFFA500UL;  // Orange color for hot
    const uint32_t red = 0xFFFF0000UL;     // Red color for very hot
    const uint32_t white = 0xFFFFFFFFUL;   // White color for extremely hot

    uint8_t redValue, greenValue, blueValue;
    if (temp <= 2200) {
		// Dark Blue to Blue
		float ratio = (float)temp / 2200.0;

		redValue = (uint8_t)((1.0 - ratio) * ((darkBlue >> 16) & 0xFF) + ratio * ((blue >> 16) & 0xFF));
		greenValue = (uint8_t)((1.0 - ratio) * ((darkBlue >> 8) & 0xFF) + ratio * ((blue >> 8) & 0xFF));
		blueValue = (uint8_t)((1.0 - ratio) * (darkBlue & 0xFF) + ratio * (blue & 0xFF));
	} else if (temp <= 2400) {
        // Blue to light Blue
		float ratio = (float)(temp - 2200) / 200.0;

        redValue = (uint8_t)((1.0 - ratio) * ((blue >> 16) & 0xFF) + ratio * ((lightBlue >> 16) & 0xFF));
        greenValue = (uint8_t)((1.0 - ratio) * ((blue >> 8) & 0xFF) + ratio * ((lightBlue >> 8) & 0xFF));
        blueValue = (uint8_t)((1.0 - ratio) * (blue & 0xFF) + ratio * (lightBlue & 0xFF));
    } else if (temp <= 2600) {
        // Light Blue to lightest blue
		float ratio = (float)(temp - 2400) / 200.0;

        redValue = (uint8_t)((1.0 - ratio) * ((lightBlue >> 16) & 0xFF) + ratio * ((lightestBlue >> 16) & 0xFF));
        greenValue = (uint8_t)((1.0 - ratio) * ((lightBlue >> 8) & 0xFF) + ratio * ((lightestBlue >> 8) & 0xFF));
        blueValue = (uint8_t)((1.0 - ratio) * (lightBlue & 0xFF) + ratio * (lightestBlue & 0xFF));
    } else if (temp <= 2800) {
        // lightest blue to purple
        float ratio = (float)(temp - 2600) / 200.0;

        redValue = (uint8_t)((1.0 - ratio) * ((lightestBlue >> 16) & 0xFF) + ratio * ((purple >> 16) & 0xFF));
        greenValue = (uint8_t)((1.0 - ratio) * ((lightestBlue >> 8) & 0xFF) + ratio * ((purple >> 8) & 0xFF));
        blueValue = (uint8_t)((1.0 - ratio) * (lightestBlue & 0xFF) + ratio * (purple & 0xFF));
    } else if (temp <= 3000) {
        // purple to Yellow
        float ratio = (float)(temp - 2800) / 200.0;

        redValue = (uint8_t)((1.0 - ratio) * ((purple >> 16) & 0xFF) + ratio * ((yellow >> 16) & 0xFF));
        greenValue = (uint8_t)((1.0 - ratio) * ((purple >> 8) & 0xFF) + ratio * ((yellow >> 8) & 0xFF));
        blueValue = (uint8_t)((1.0 - ratio) * (purple & 0xFF) + ratio * (yellow & 0xFF));
    } else if (temp <= 3250) {
        // Yellow to Orange
        float ratio = (float)(temp - 3000) / 200.0;

        redValue = (uint8_t)((1.0 - ratio) * ((yellow >> 16) & 0xFF) + ratio * ((orange >> 16) & 0xFF));
        greenValue = (uint8_t)((1.0 - ratio) * ((yellow >> 8) & 0xFF) + ratio * ((orange >> 8) & 0xFF));
        blueValue = (uint8_t)((1.0 - ratio) * (yellow & 0xFF) + ratio * (orange & 0xFF));
    } else if (temp <= 3500) {
        // Orange to Red
        float ratio = (float)(temp - 3250) / 250.0;

        redValue = (uint8_t)((1.0 - ratio) * ((orange >> 16) & 0xFF) + ratio * ((red >> 16) & 0xFF));
        greenValue = (uint8_t)((1.0 - ratio) * ((orange >> 8) & 0xFF) + ratio * ((red >> 8) & 0xFF));
        blueValue = (uint8_t)((1.0 - ratio) * (orange & 0xFF) + ratio * (red & 0xFF));
    } else {
        // Red to White
        float ratio = (float)(temp - 3500) / 1500.0;

        redValue = (uint8_t)((1.0 - ratio) * ((red >> 16) & 0xFF) + ratio * ((white >> 16) & 0xFF));
        greenValue = (uint8_t)((1.0 - ratio) * ((red >> 8) & 0xFF) + ratio * ((white >> 8) & 0xFF));
        blueValue = (uint8_t)((1.0 - ratio) * (red & 0xFF) + ratio * (white & 0xFF));
    }

    // Combine into a single color value
    uint32_t color = (0xFF << 24) | (redValue << 16) | (greenValue << 8) | blueValue;

    return color;
}



// Helper function to interpolate between two colors
uint32_t interpolateColor(uint32_t c1, uint32_t c2, float t) {
    uint8_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;

    uint8_t r = (uint8_t)((1 - t) * r1 + t * r2);
    uint8_t g = (uint8_t)((1 - t) * g1 + t * g2);
    uint8_t b = (uint8_t)((1 - t) * b1 + t * b2);

    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}



// Bilinear interpolation of colors in a 2x2 grid
uint32_t bilinearInterpolate(uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br, float tx, float ty) {
    uint32_t top = interpolateColor(tl, tr, tx);
    uint32_t bottom = interpolateColor(bl, br, tx);
    return interpolateColor(top, bottom, ty);
}



void displayMatrixOnLCD(uint32_t matrix[12][16]) {
    for (int y = 0; y < MATRIX_SIZE_Y; y++) {
        for (int x = 0; x < MATRIX_SIZE_X; x++) {
            uint16_t temp = matrix[y][x];

            // Limit temperature to the range 0 to 5000
            if (temp > 5000) temp = 5000;
            if (temp < 0) temp = 0;

            // Convert temperature to color
            matrix[y][x] = tempToColor(temp);
        }
    }

    uint32_t x_size, y_size;
    BSP_LCD_GetXSize(0, &x_size);
    BSP_LCD_GetYSize(0, &y_size);



    // Determine the scaling factors and offsets to maintain aspect ratio
    float scale_x, scale_y, offset_x, offset_y;

	// Screen is wider than the matrix
	scale_y = (float)y_size / (float)MATRIX_SIZE_Y;
	scale_x = scale_y;
	offset_x = (x_size - (MATRIX_SIZE_X * scale_x)) / 2;
	offset_y = 0;

    // Render each pixel on the LCD (image pixel)
    for (uint32_t py = 0; py < y_size; py++) {
        for (uint32_t px = offset_x; px < x_size - offset_x; px++) {
            // Calculate the grid cell the pixel falls into, adjusted by offset
            int cell_x = (int)((px - offset_x) / scale_x);
            int cell_y = (int)((py - offset_y) / scale_y);


            // Calculate the interpolation factors
            float tx = ((px - offset_x) / scale_x) - cell_x;
            float ty = ((py - offset_y) / scale_y) - cell_y;

            // Get the colors of the four surrounding cells
            uint32_t top_left = matrix[cell_y][cell_x];
            uint32_t top_right = (cell_x < MATRIX_SIZE_X - 1) ? matrix[cell_y][cell_x + 1] : top_left;
            uint32_t bottom_left = (cell_y < MATRIX_SIZE_Y - 1) ? matrix[cell_y + 1][cell_x] : top_left;
            uint32_t bottom_right = top_left;	//bottom right has more exceptions
            if (cell_x < MATRIX_SIZE_X - 1 && cell_y < MATRIX_SIZE_Y - 1) {
            	bottom_right = matrix[cell_y + 1][cell_x + 1];

            } else if (cell_x >= MATRIX_SIZE_X - 1 && cell_y < MATRIX_SIZE_Y - 1) {
            	bottom_right = matrix[cell_y + 1][cell_x];

            } else if (cell_x < MATRIX_SIZE_X - 1 && cell_y >= MATRIX_SIZE_Y - 1) {
            	bottom_right = matrix[cell_y][cell_x + 1];
            }


            // Interpolate the color
            uint32_t color = bilinearInterpolate(top_left, top_right, bottom_left, bottom_right, tx, ty);

            // Set the pixel
            UTIL_LCD_SetPixel(px, py, color);

        }
    }
}





/**
  * @brief  Draw Touchscreen Background
  * @param  state : touch zone state
  * @retval None
  */
void Touchscreen_DrawBackground_Circles(uint8_t state)
{
  uint16_t x, y;
  switch (state)
  {
    case 0:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_RED);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_YELLOW);


      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_GREEN);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      break;

    case 1:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      break;

    case 2:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_RED);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      break;

    case 4:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_YELLOW);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      break;

    case 8:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(4), CIRCLE_RADIUS, UTIL_LCD_COLOR_GREEN);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(4), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      break;

    case 16:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);

      UTIL_LCD_DrawHLine(CIRCLE_XPOS(1)-LINE_LENGHT, CIRCLE_YPOS(1), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawHLine(CIRCLE_XPOS(2)-LINE_LENGHT, CIRCLE_YPOS(2), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawVLine(CIRCLE_XPOS(2), CIRCLE_YPOS(2)-LINE_LENGHT, 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawHLine(CIRCLE_XPOS(3)-LINE_LENGHT, CIRCLE_YPOS(3), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawHLine(CIRCLE_XPOS(4)-LINE_LENGHT, CIRCLE_YPOS(4), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawVLine(CIRCLE_XPOS(4), CIRCLE_YPOS(4)-LINE_LENGHT, 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);

      UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_SetFont(&Font24);
      x = CIRCLE_XPOS(1);
      y = CIRCLE_YPOS(1) - CIRCLE_RADIUS - UTIL_LCD_GetFont()->Height;
      UTIL_LCD_DisplayStringAt(x, y, (uint8_t *)"Volume", CENTER_MODE);
      x = CIRCLE_XPOS(4);
      y = CIRCLE_YPOS(4) - CIRCLE_RADIUS - UTIL_LCD_GetFont()->Height;
      UTIL_LCD_DisplayStringAt(x, y, (uint8_t *)"Frequency", CENTER_MODE);

      break;

    case 32:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLACK);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLACK);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);


      UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLACK);
      UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_SetFont(&Font20);
      x = CIRCLE_XPOS(1) - 10;
      y = CIRCLE_YPOS(1) - (UTIL_LCD_GetFont()->Height)/2;
      UTIL_LCD_DisplayStringAt(x, y, (uint8_t *)"Up", LEFT_MODE);
      x = CIRCLE_XPOS(2) - 10;
      y = CIRCLE_YPOS(3)  - (UTIL_LCD_GetFont()->Height)/2;
      UTIL_LCD_DisplayStringAt(x, y, (uint8_t *)"Dw", LEFT_MODE);
      UTIL_LCD_SetFont(&Font12);

      break;

    case 64:
      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS, UTIL_LCD_COLOR_BLUE);

      UTIL_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2, UTIL_LCD_COLOR_WHITE);

      UTIL_LCD_DrawHLine(CIRCLE_XPOS(2)-LINE_LENGHT, CIRCLE_YPOS(2), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawHLine(CIRCLE_XPOS(3)-LINE_LENGHT, CIRCLE_YPOS(3), 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_DrawVLine(CIRCLE_XPOS(3), CIRCLE_YPOS(3)-LINE_LENGHT, 2*LINE_LENGHT, UTIL_LCD_COLOR_BLUE);

      UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLUE);
      UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_WHITE);
      UTIL_LCD_SetFont(&Font24);
      x = CIRCLE_XPOS(2);
      y = CIRCLE_YPOS(1) - CIRCLE_RADIUS - UTIL_LCD_GetFont()->Height;
      UTIL_LCD_DisplayStringAt(x, y, (uint8_t *)"Frequency", LEFT_MODE);
      break;
  }
}

/**
  * @brief  TouchScreen get touch position
  * @param  None
  * @retval None
  */
uint8_t TouchScreen_GetTouchPosition(void)
{
  uint16_t x1, y1;
  uint8_t circleNr = 0;

  /* Check in polling mode in touch screen the touch status and coordinates */
  /* of touches if touch occurred                                           */
  BSP_TS_GetState(0, &TS_State);
  if(TS_State.TouchDetected)
  {
    /* One or dual touch have been detected          */
    /* Only take into account the first touch so far */

    /* Get X and Y position of the first */
    x1 = TS_State.TouchX;
    y1 = TS_State.TouchY;

    if ((y1 > (CIRCLE_YPOS(1) - CIRCLE_RADIUS)) &&
        (y1 < (CIRCLE_YPOS(1) + CIRCLE_RADIUS)))
    {
      if ((x1 > (CIRCLE_XPOS(1) - CIRCLE_RADIUS)) &&
          (x1 < (CIRCLE_XPOS(1) + CIRCLE_RADIUS)))
      {
        circleNr = 1;
      }
      if ((x1 > (CIRCLE_XPOS(2) - CIRCLE_RADIUS)) &&
          (x1 < (CIRCLE_XPOS(2) + CIRCLE_RADIUS)))
      {
        circleNr = 2;
      }

      if ((x1 > (CIRCLE_XPOS(3) - CIRCLE_RADIUS)) &&
          (x1 < (CIRCLE_XPOS(3) + CIRCLE_RADIUS)))
      {
        circleNr = 3;
      }

      if ((x1 > (CIRCLE_XPOS(4) - CIRCLE_RADIUS)) &&
          (x1 < (CIRCLE_XPOS(4) + CIRCLE_RADIUS)))
      {
        circleNr = 4;
      }
    }
    else
    {
      if (((y1 < 220) && (y1 > 140)) &&
          ((x1 < 160) && (x1 > 100)))
      {
        circleNr = 0xFE;   /* top part of the screen */
      }
      else
      {
        circleNr = 0xFF;
      }
    }
  } /* of if(TS_State.TouchDetected) */
  return circleNr;
}

/**
  * @}
  */

/**
  * @}
  */
