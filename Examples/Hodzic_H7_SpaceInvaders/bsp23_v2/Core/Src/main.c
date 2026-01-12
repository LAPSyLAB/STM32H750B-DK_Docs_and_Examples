/**
  ******************************************************************************
  * @file    BSP/Src/main.c
  * @author  MCD Application Team
  * @brief   This example code shows how to use the STM32H750B_DISCOVERY BSP Drivers
  *          This is the main program.   
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
#include "stm32h7xx_hal.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup BSP
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */

// Spaceship
typedef struct {
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
	uint32_t color;
} Spaceship;

void drawSpaceship(Spaceship spaceship) {
    // Draw a triangle instead of a rectangle
    Point points[3];
    points[0].X = spaceship.x;
    points[0].Y = spaceship.y;
    points[1].X = spaceship.x - spaceship.width / 2;
    points[1].Y = spaceship.y + spaceship.height;
    points[2].X = spaceship.x + spaceship.width / 2;
    points[2].Y = spaceship.y + spaceship.height;

    UTIL_LCD_FillPolygon(points, 3, spaceship.color);
}

void clearTriangleArea(Spaceship spaceship, uint32_t y_size) {
    uint32_t minX = spaceship.x - spaceship.width / 2;
    uint32_t minY = spaceship.y;
    uint32_t maxX = spaceship.x + spaceship.width / 2;
    uint32_t maxY = spaceship.y + spaceship.height;

    redrawBackgroundArea(minX, minY, maxX - minX + 1, maxY - minY + 1, y_size);
}

void moveLeft(Spaceship *spaceship, uint32_t step, uint32_t y_size) {
    if (spaceship->x > step + spaceship->width / 2) {
        clearTriangleArea(*spaceship, y_size);
        spaceship->x -= step;
        drawSpaceship(*spaceship);
    }
}

void moveRight(Spaceship *spaceship, uint32_t step, uint32_t x_size, uint32_t y_size) {
    if (spaceship->x + step < x_size - spaceship->width / 2) {
        clearTriangleArea(*spaceship, y_size);
        spaceship->x += step;
        drawSpaceship(*spaceship);
    }
}

// Invader
#define MAX_INVADERS 10
#define INITIAL_INVADERS 5
#define INITIAL_INVADER_SPEED 5
#define INVADER_INCREMENT_THRESHOLD 10

typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t radius;
  uint32_t color;
} Invader;


void drawInvader(Invader invader) {
    if (invader.x != UINT32_MAX) {
        // Draw a circle for the invader's head
        UTIL_LCD_FillCircle(invader.x, invader.y, invader.radius, invader.color);
        // Draw eyes
        UTIL_LCD_FillCircle(invader.x - invader.radius/2, invader.y - invader.radius/2, invader.radius/5, UTIL_LCD_COLOR_WHITE);
        UTIL_LCD_FillCircle(invader.x + invader.radius/2, invader.y - invader.radius/2, invader.radius/5, UTIL_LCD_COLOR_WHITE);
        // Draw a simple body
        UTIL_LCD_DrawRect(invader.x - invader.radius/2, invader.y + invader.radius, invader.radius, invader.radius, invader.color);
    }
}

void drawInvaders(Invader *invaders, int num_invaders) {
	for (int i = 0; i < num_invaders; i++) {
		drawInvader(invaders[i]);
	}
}

void moveInvader(Invader *invader, uint32_t step, uint32_t x_size, uint32_t y_size) {
    if (invader->x != UINT32_MAX) {
        // Clear a larger area to ensure complete removal
        uint32_t clearWidth = 2 * invader->radius + 2;  // Add some extra pixels for safety
        uint32_t clearHeight = 3 * invader->radius + step + 2;  // Account for head and body, plus movement

        redrawBackgroundArea(invader->x - invader->radius - 1,
                             invader->y - invader->radius - 1,
                             clearWidth,
                             clearHeight,
                             y_size);

        invader->y += step;
        drawInvader(*invader);
    }
}

void removeInvader(Invader *invader, uint32_t y_size) {
    if (invader->x != UINT32_MAX) {
        // Clear a much larger area to ensure complete removal
        uint32_t clearWidth = 4 * invader->radius;
        uint32_t clearHeight = 4 * invader->radius;

        redrawBackgroundArea(invader->x - 2 * invader->radius,
                             invader->y - 2 * invader->radius,
                             clearWidth,
                             clearHeight,
                             y_size);

        invader->x = UINT32_MAX; // Mark the invader as removed
    }
}

bool invaderReachedBottom(Invader invader, uint32_t y_size, uint32_t spaceship_y) {
    // Check if the bottom of the invader (including its body) has reached or passed the spaceship's y position
    return (invader.y + 2 * invader.radius >= spaceship_y);
}

// Bounding box
typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;
} BoundingBox;

// Check collision between spaceship and invader
int checkCollision(Spaceship spaceship, Invader invader) {
  if (invader.x == UINT32_MAX) {
	return 0;
  }

  BoundingBox spaceship_bb = {spaceship.x, spaceship.y, spaceship.width, spaceship.height};
  BoundingBox invader_bb = {invader.x - invader.radius, invader.y - invader.radius, 2 * invader.radius, 2 * invader.radius};
  if (spaceship_bb.x < invader_bb.x + invader_bb.width &&
      spaceship_bb.x + spaceship_bb.width > invader_bb.x &&
      spaceship_bb.y < invader_bb.y + invader_bb.height &&
      spaceship_bb.y + spaceship_bb.height > invader_bb.y) {
    return 1;
  }
  return 0;
}

// Bullet
typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t radius;
  uint32_t color;
  bool is_fired;
  bool hit;
} Bullet;

#define MAX_BULLETS 10
#define BULLET_STEP 5

void fireBullet(Bullet *bullets, uint32_t x, uint32_t y, uint32_t radius, uint32_t color) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].is_fired) {
            bullets[i].x = x;
            bullets[i].y = y;
            bullets[i].radius = radius;
            bullets[i].color = color;
            bullets[i].is_fired = true;
            bullets[i].hit = false;
            UTIL_LCD_FillCircle(x, y, radius, color);
            break;
        }
    }
}

void updateBullets(Bullet *bullets, uint32_t y_size, Invader *invaders, uint32_t num_invaders, uint32_t *score, int lives) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].is_fired) {
            // Clear the old bullet and redraw background
            redrawBackgroundArea(bullets[i].x - bullets[i].radius,
                                  bullets[i].y - bullets[i].radius,
                                  2 * bullets[i].radius + 1,
                                  2 * bullets[i].radius + BULLET_STEP + 1, y_size);

            bullets[i].y -= BULLET_STEP;

            if (bullets[i].y < bullets[i].radius) {
                bullets[i].is_fired = false;
                continue; // Skip drawing this bullet
            }

            bool hit_invader = false;
            for (uint32_t j = 0; j < num_invaders; j++) {
                if (invaders[j].x != UINT32_MAX && checkBulletCollision(bullets[i], invaders[j])) {
                    removeInvader(&invaders[j], y_size);
                    hit_invader = true;
                    (*score)++; // Increment score when an invader is hit
                    displayGameStats(*score, lives); // Update the score display
                    break;
                }
            }

            if (hit_invader) {
                // Clear the bullet and mark it as not fired
                redrawBackgroundArea(bullets[i].x - bullets[i].radius,
                                     bullets[i].y - bullets[i].radius,
                                     2 * bullets[i].radius + 1,
                                     2 * bullets[i].radius + 1, y_size);
                bullets[i].is_fired = false;
            } else {
                // Draw the new bullet
                UTIL_LCD_FillCircle(bullets[i].x, bullets[i].y, bullets[i].radius, bullets[i].color);
            }
        }
    }
}


// Check collision between bullet and invader
int checkBulletCollision(Bullet bullet, Invader invader) {
	if (invader.x == UINT32_MAX) {
		return 0;
	}

	BoundingBox bullet_bb = { bullet.x - bullet.radius, bullet.y
			- bullet.radius, 2 * bullet.radius, 2 * bullet.radius };
	BoundingBox invader_bb = { invader.x - invader.radius, invader.y
			- invader.radius, 2 * invader.radius, 2 * invader.radius };
	if (bullet_bb.x < invader_bb.x + invader_bb.width
			&& bullet_bb.x + bullet_bb.width > invader_bb.x
			&& bullet_bb.y < invader_bb.y + invader_bb.height
			&& bullet_bb.y + bullet_bb.height > invader_bb.y) {
		return 1;
	}
	return 0;
}

void resetBullets(Bullet *bullets) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].is_fired = false;
        bullets[i].hit = false;
    }
}


// Invader helper functions
#define MIN_INVADER_DISTANCE 100  // Minimum distance between invaders
#define MAX_SPAWN_ATTEMPTS 100   // Maximum attempts to find a valid position

// Helper function to calculate distance between two points
uint32_t calculateDistance(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
    int32_t dx = (int32_t)x1 - (int32_t)x2;
    int32_t dy = (int32_t)y1 - (int32_t)y2;
    return sqrt(dx*dx + dy*dy);
}

// Helper function to check if a position is valid
bool isValidPosition(uint32_t x, uint32_t y, Invader *invaders, uint32_t current_invader) {
    for (uint32_t i = 0; i < current_invader; i++) {
        if (calculateDistance(x, y, invaders[i].x, invaders[i].y) < MIN_INVADER_DISTANCE) {
            return false;
        }
    }
    return true;
}

void moveInvaders(Invader *invaders, int num_invaders, uint32_t step, uint32_t x_size, uint32_t speed, uint32_t y_size) {
  for (int i = 0; i < num_invaders; i++) {
    moveInvader(&invaders[i], speed, x_size, y_size);
  }
}

void initInvaders(Invader *invaders, uint32_t num_invaders, uint32_t x_size, uint32_t y_size) {
    srand(HAL_GetTick());  // Seed the random number generator with the current tick

    for (uint32_t i = 0; i < num_invaders; i++) {
        uint32_t attempts = 0;
        uint32_t x, y;
        bool valid_position = false;

        while (!valid_position && attempts < MAX_SPAWN_ATTEMPTS) {
            x = rand() % (x_size - 20) + 10;  // Random x position, ensuring invader is within screen bounds
            y = rand() % (y_size / 3);        // Random y position, keeping invaders in the top third of the screen

            valid_position = isValidPosition(x, y, invaders, i);
            attempts++;
        }

        if (valid_position) {
            invaders[i].x = x;
            invaders[i].y = y;
        } else {
            // If we couldn't find a valid position after MAX_SPAWN_ATTEMPTS,
            // place the invader at a default position
            invaders[i].x = 10 + (i * 20) % (x_size - 20);
            invaders[i].y = 10 + ((i * 20) / (x_size - 20)) * 20;
        }

        invaders[i].radius = 10;
        invaders[i].color = UTIL_LCD_COLOR_ORANGE;
    }
}

// Utility function to clear an invader
void clearInvader(Invader invader) {
    UTIL_LCD_FillCircle(invader.x, invader.y, invader.radius, UTIL_LCD_COLOR_WHITE);
}

// Function to reset invaders' positions and states when moving to the next level
void resetInvaders(Invader *invaders, uint32_t num_invaders, uint32_t x_size, uint32_t y_size) {
    for (uint32_t i = 0; i < num_invaders; i++) {
        clearInvader(invaders[i]);  // Clear old invader
        invaders[i].x = rand() % (x_size - 20) + 10;  // Random x position, ensuring invader is within screen bounds
        invaders[i].y = rand() % (y_size / 3);  // Random y position, keeping invaders in the top third of the screen
        invaders[i].radius = 10;
        invaders[i].color = UTIL_LCD_COLOR_ORANGE;
    }
}

// Function to check if all invaders are cleared
int allInvadersCleared(Invader *invaders, int num_invaders) {
  for (int i = 0; i < num_invaders; i++) {
    if (invaders[i].x != UINT32_MAX) {
      return 0;
    }
  }
  return 1;
}


// Score and lives
void displayGameStats(uint32_t score, int lives) {
    char stats[50];
    snprintf(stats, 50, "Score: %u            Lives: %d", score, lives);
    UTIL_LCD_SetFont(&Font24);
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_BLACK);
    UTIL_LCD_DisplayStringAt(0, 0, (uint8_t *)stats, LEFT_MODE);
}


void displayHighScore(uint32_t high_score, uint32_t y_size) {
    char high_score_text[50];
    snprintf(high_score_text, 50, "High Score: %u", high_score);
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_RED);
    UTIL_LCD_DisplayStringAt(0, 50, (uint8_t *)high_score_text, CENTER_MODE);
}

// Stars
#define MAX_STARS 100

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t brightness;
} Star;

Star stars[MAX_STARS];

void initStars(uint32_t x_size, uint32_t y_size) {
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = rand() % x_size;
        stars[i].y = rand() % y_size;
        stars[i].brightness = UTIL_LCD_COLOR_WHITE; // All stars are white
    }
}

void drawStar(uint32_t x, uint32_t y, uint32_t color) {
    UTIL_LCD_FillRect(x, y, 1, 1, color); // Draw a "pixel" as a 1x1 rectangle
}

// Background
void drawGradientBackground(uint32_t x_size, uint32_t y_size) {
    for (uint32_t y = 0; y < y_size; y++) {
        uint32_t color = UTIL_LCD_COLOR_BLUE + ((y * 255 / y_size) << 16); // Gradient from blue to black
        UTIL_LCD_DrawHLine(0, y, x_size, color);
    }
}

void drawBackground(uint32_t x_size, uint32_t y_size) {
    drawGradientBackground(x_size, y_size);
    for (int i = 0; i < MAX_STARS; i++) {
        drawStar(stars[i].x, stars[i].y, stars[i].brightness);
    }
}

void redrawBackgroundArea(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t y_size) {
    for (uint32_t cy = y; cy < y + height; cy++) {
        uint32_t color = UTIL_LCD_COLOR_BLUE + ((cy * 255 / y_size) << 16);
        UTIL_LCD_DrawHLine(x, cy, width, color);
    }

    for (int i = 0; i < MAX_STARS; i++) {
        if (stars[i].x >= x && stars[i].x < x + width &&
            stars[i].y >= y && stars[i].y < y + height) {
            drawStar(stars[i].x, stars[i].y, stars[i].brightness);
        }
    }
}

// HAL GPIO
#define BUTTON_GPIO_PORT GPIOC
#define BUTTON_PIN GPIO_PIN_13
#define BUTTON_PRESSED GPIO_PIN_RESET

void Button_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE(); // Enable GPIOC clock

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BUTTON_GPIO_PORT, &GPIO_InitStruct);
}


int main(void) {
    /* Enable the CPU Cache */
    CPU_CACHE_Enable();

    /* STM32H7xx HAL library initialization */
    HAL_Init();

    /* Configure the system clock to 400 MHz */
    SystemClock_Config();

    // LCD initialization
    uint32_t x_size, y_size;
    BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);
    UTIL_LCD_SetFuncDriver(&LCD_Driver);
    UTIL_LCD_SetLayer(0);
    UTIL_LCD_Clear(UTIL_LCD_COLOR_WHITE);
    BSP_LCD_GetXSize(0, &x_size);
    BSP_LCD_GetYSize(0, &y_size);

    // Touchscreen initialization
    TS_Init_t hTS;
    hTS.Width = x_size;
    hTS.Height = y_size;
    hTS.Orientation = TS_SWAP_XY;
    hTS.Accuracy = 5;
    BSP_TS_Init(0, &hTS);
    Button_Init();
    static TS_State_t TS_State;


    Spaceship spaceship = {x_size / 2, y_size - 20, 10, 20, UTIL_LCD_COLOR_BLACK};
    Bullet bullets[MAX_BULLETS] = {0};
    Invader invaders[MAX_INVADERS] = {0};

    uint32_t high_score = 0;
    uint32_t score = 0;
    uint32_t invader_speed = INITIAL_INVADER_SPEED;
    uint32_t num_invaders = INITIAL_INVADERS;
    initInvaders(invaders, num_invaders, x_size, y_size);

    drawSpaceship(spaceship);
    drawInvaders(invaders, num_invaders);

    uint32_t last_update = HAL_GetTick();
    uint32_t update_interval = 20;
    uint32_t invader_interval = 1000;
    uint32_t last_invader_update = HAL_GetTick();

    GPIO_PinState previousButtonState = GPIO_PIN_SET;

    initStars(x_size, y_size);
    drawBackground(x_size, y_size);

    int lives = 3;
    displayGameStats(score, lives);
    while (lives > 0) {
        uint32_t current_time = HAL_GetTick();
        BSP_TS_GetState(0, &TS_State);

        GPIO_PinState currentButtonState = HAL_GPIO_ReadPin(BUTTON_GPIO_PORT, BUTTON_PIN);

        if (currentButtonState == BUTTON_PRESSED && previousButtonState != BUTTON_PRESSED) {
            fireBullet(bullets, spaceship.x + spaceship.width / 2, spaceship.y, 5, UTIL_LCD_COLOR_BLACK);
        }

        previousButtonState = currentButtonState;

        if (current_time - last_update >= update_interval) {
            if (TS_State.TouchDetected) {
                if (TS_State.TouchX < x_size / 2) {
                    moveLeft(&spaceship, 5, y_size);
                } else {
                    moveRight(&spaceship, 5, x_size, y_size);
                }
            }
            updateBullets(bullets, y_size, invaders, num_invaders, &score, lives);
            last_update = current_time;
        }

        if (current_time - last_invader_update >= invader_interval) {
            moveInvaders(invaders, num_invaders, 5, x_size, invader_speed, y_size);
            last_invader_update = current_time;
        }

        // Check if any invader has reached the bottom
        for (int i = 0; i < num_invaders; i++) {
            if (invaders[i].x != UINT32_MAX && invaderReachedBottom(invaders[i], y_size, spaceship.y)) {
                lives--;
                removeInvader(&invaders[i], y_size);  // Remove the invader that reached the bottom

                // VIsual feedback when losing a life
                // Flash the screen red
                UTIL_LCD_Clear(UTIL_LCD_COLOR_RED);
                HAL_Delay(200);
                drawBackground(x_size, y_size);
                drawSpaceship(spaceship);
                drawInvaders(invaders, num_invaders);
            }
        }

        for (int i = 0; i < num_invaders; i++) {
            if (checkCollision(spaceship, invaders[i])) {
            	lives--;
                UTIL_LCD_Clear(UTIL_LCD_COLOR_RED);
                HAL_Delay(200);
                drawBackground(x_size, y_size);
                drawSpaceship(spaceship);
                drawInvaders(invaders, num_invaders);
            	if (lives == 0) {
            		UTIL_LCD_Clear(UTIL_LCD_COLOR_WHITE);
            		UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_RED);
            		UTIL_LCD_DisplayStringAt(x_size / 2, y_size / 2, (uint8_t *)"Game Over", CENTER_MODE);
            		if (score > high_score) {
            			high_score = score;
            		}
            		return 0;
            	}
            	invaders[i].x = UINT32_MAX;
            }
        }

        if (allInvadersCleared(invaders, num_invaders)) {
            if (score % INVADER_INCREMENT_THRESHOLD == 0 && num_invaders < MAX_INVADERS) {
                num_invaders++;  // Increase the number of invaders
                invader_speed += 2;  // Increase the speed of invaders
                initInvaders(invaders, num_invaders, x_size, y_size);
            }
            resetBullets(bullets);
            resetInvaders(invaders, num_invaders, x_size, y_size);
            redrawBackgroundArea(0, 0, x_size, y_size, y_size);
            drawSpaceship(spaceship);
            drawInvaders(invaders, num_invaders);
        }
        displayGameStats(score, lives);
    }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 400000000 (Cortex-M7 CPU Clock)
  *            HCLK(Hz)                       = 200000000 (Cortex-M4 CPU, Bus matrix Clocks)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 160
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

/* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

 /*
  Note : The activation of the I/O Compensation Cell is recommended with communication  interfaces
          (GPIO, SPI, FMC, QSPI ...)  when  operating at  high frequencies(please refer to product datasheet)
          The I/O Compensation Cell activation  procedure requires :
        - The activation of the CSI clock
        - The activation of the SYSCFG clock
        - Enabling the I/O Compensation Cell : setting bit[0] of register SYSCFG_CCCSR
 */

  /*activate CSI clock mondatory for I/O Compensation Cell*/
  __HAL_RCC_CSI_ENABLE() ;

  /* Enable SYSCFG clock mondatory for I/O Compensation Cell */
  __HAL_RCC_SYSCFG_CLK_ENABLE() ;

  /* Enables the I/O Compensation Cell */
  HAL_EnableCompensationCell();
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* Turn LED REDon */
  BSP_LED_On(LED_RED);
  while(1)
  {
  }
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @}
  */

/**
  * @}
  */

