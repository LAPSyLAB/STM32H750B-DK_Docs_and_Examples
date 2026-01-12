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
#include "cmsis_os.h"
#include <stdbool.h>
#include <stdlib.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "stlogo.h"
#include "app_timers.h"
#include "semphr.h"

#include "uart.h"
#include "retarget.h"
#include "dma.h"

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

// handles
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim6;
ADC_HandleTypeDef hadc3;
DMA_HandleTypeDef hdmaDisplay;

uint32_t ARGB8888(uint8_t a, uint8_t r, uint8_t g, uint8_t b);
uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b);

enum HitBoxTriggerable
{
  PRESS,
  LONGPRESS,
  DRAG
};

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 32,
    .priority = (osPriority_t)osPriorityNormal,
};

/* Definitions for secondTask */
osThreadId_t screenTaskHandle;
const osThreadAttr_t screenTask_attributes = {
    .name = "screenTask",
    .stack_size = 128 * 32,
    .priority = (osPriority_t)osPriorityNormal,
};

osThreadId_t touchTaskHandle;
const osThreadAttr_t touchTask_attributes = {
    .name = "touchTask",
    .stack_size = 128 * 8,
    .priority = (osPriority_t)osPriorityNormal,
};

osThreadId_t simTaskHandle;
const osThreadAttr_t simTask_attributes = {
    .name = "simTask",
    .stack_size = 128 * 16,
    .priority = (osPriority_t)osPriorityNormal,
};

/* USER CODE BEGIN PV */
__IO uint32_t ButtonState = 0;
uint32_t x_size;
uint32_t y_size;
TS_State_t TS_State_display;
TS_Init_t hTS_display;

// logic functions

typedef bool (*LogicFunction)(bool *inputStates, int numInputs);

bool logic_and(bool *inputStates, int numInputs)
{
  for (int i = 0; i < numInputs; i++)
  {
    if (inputStates[i] == false)
      return false;
  }
  return true;
}
bool logic_true(bool *inputStates, int numInputs)
{
  return true;
}
bool logic_false(bool *inputStates, int numInputs)
{
  return false;
}

bool logic_or(bool *inputStates, int numInputs)
{
  for (int i = 0; i < numInputs; i++)
  {
    if (inputStates[i] == true)
      return true;
  }
  return false;
}

bool logic_xor(bool *inputStates, int numInputs)
{
  int count = 0;
  for (int i = 0; i < numInputs; i++)
  {
    if (inputStates[i])
      count++;
  }
  return (count % 2) == 1;
}

bool logic_xand(bool *inputStates, int numInputs)
{
  int count = 0;
  for (int i = 0; i < numInputs; i++)
  {
    if (inputStates[i])
      count++;
  }
  return !(count == 1);
}

bool logic_xnor(bool *inputStates, int numInputs)
{
  int count = 0;
  for (int i = 0; i < numInputs; i++)
  {
    if (inputStates[i])
      count++;
  }
  return (count % 2) == 0;
}

bool logic_nor(bool *inputStates, int numInputs)
{
  for (int i = 0; i < numInputs; i++)
  {
    if (inputStates[i])
      return false;
  }
  return true;
}

bool logic_nand(bool *inputStates, int numInputs)
{
  for (int i = 0; i < numInputs; i++)
  {
    if (!inputStates[i])
      return true;
  }
  return false;
}

bool logic_not(bool *inputStates, int numInputs)
{
  return !inputStates[0];
}

// logic functions end

#define MAX_NAME_LENGTH 5
#define MAX_ELEMENTS 24
#define MAX_IO 4
#define IO_CIRCLE_SIZE 10
#define IO_PIN_SIZE 17

#define LCD_WIDTH 480
#define LCD_HEIGHT 272
#define BYTES_PER_PIXEL 2
#define FRAMEBUFFER_SIZE (LCD_WIDTH * LCD_HEIGHT * BYTES_PER_PIXEL)

#define HALF_HEIGHT (LCD_HEIGHT / 2)
size_t oneBufferSize = FRAMEBUFFER_SIZE / 2;

#define LCD_FRAME_BUFFER 0xD0000000U
uint32_t *fb = (uint32_t *)LCD_FRAME_BUFFER;

typedef struct LogicElement LogicElement;
typedef struct io io;

typedef struct HitBox
{
  bool active;
  int x, y;
  int width, height;
  int indexParam;
  bool freeable;
  enum HitBoxTriggerable triggerable;
  void (*onClick)(int);
} HitBox;

typedef struct io
{
  int xOffset, yOffset;
  bool isInput;
  HitBox ioHitbox;
  LogicElement *parent;
  io **connections;
  int numConnections;
} io;

typedef struct LogicElement
{
  bool active;
  int id;
  char name[MAX_NAME_LENGTH];
  int x, y;
  int numInputs;
  int numOutputs;
  io *inputs;
  io *outputs;
  bool currentState;
  bool nextState;
  HitBox hitbox;
  LogicFunction logicCallback;
} LogicElement;

typedef struct UILogicElement
{
  int id;
  bool active;
  char name[MAX_NAME_LENGTH];
  LogicElement *element;
  // HitBox hitbox;
} UILogicElement;

UILogicElement *uiElementsList;
LogicElement *definedElementsList;
LogicElement *placedElementsList;
LogicElement *otherButtonsList;

LogicElement *inputPinList;
LogicElement *outputPinList;

// semaphores
SemaphoreHandle_t displayStateMutex;

SemaphoreHandle_t drawBufferMutex;

bool isHolding = false;
float holdStartTimeStamp = -1;
int holdStartX = -1;
int holdStartY = -1;
int holdEndX = -1;
int holdEndY = -1;

int logicElementWidth = 85;
int logicElementHeight = 70;

int currentTopBarPage = 0;
int selectedeElement = 0;
int uiSelectedeElement = 0;

int globalIndex = -1;
int numInputs = 2;
int numOutputs = 1;

int numInputPins = 2;
int numOutputPins = 2;

bool ioMode = false;
bool simulating = false;

bool toggl = false;
bool needsRedraw = false;
int timeoutMs = 0;

// #define PRIMARYCOLOR 0xFF800000UL
// #define PRIMARYCOLOR RGB565(128, 0, 0)
// #define SECONDARYCOLOR 0xFF00FFFFUL
// #define SECONDARYCOLOR RGB565(0, 255, 255)

#define PRIMARYCOLOR RGB565(109, 93, 231)
#define SECONDARYCOLOR RGB565(162, 200, 240)

#define PRIMARYFOREGROUND RGB565(255, 255, 255)
#define SECONDARYFOREGROUND RGB565(51, 51, 51)

#define ONCOLOR RGB565(0, 180, 255)
#define OFFCOLOR RGB565(80, 100, 120)

#define LIGHTBLUE 0xFF00ccccUL
#define DARKBLUE 0xFF145454UL

UART_HandleTypeDef UART3Handle;
TIM_HandleTypeDef TIM3Handle;

void CreateIoHitbox(io *element, int ioCenterX, int ioCenterY, int radius, void (*onClick)(int), int indexParam);
void DummyFunction(int indexParam);
void ToggleIO(int indexParam);
void InitLeds(void);

void InitGate(LogicElement *gate, int id, const char *name, int x, int y, int numInputs, int numOutputs, LogicFunction callbackFunction)
{
  gate->active = true;
  gate->id = id;
  gate->x = x;
  gate->y = y;

  gate->logicCallback = callbackFunction;

  gate->numInputs = numInputs;
  gate->numOutputs = numOutputs;

  memset(gate->name, 0, MAX_NAME_LENGTH);
  strncpy(gate->name, name, MAX_NAME_LENGTH - 1);

  gate->inputs = malloc(numInputs * sizeof(io));
  gate->outputs = malloc(numOutputs * sizeof(io));

  if (!gate->inputs || !gate->outputs)
  {
    // handle malloc failure
    return;
  }

  for (int i = 0; i < numInputs; i++)
  {
    gate->inputs[i].xOffset = 0;
    gate->inputs[i].yOffset = 0;
    gate->inputs[i].isInput = true;
    gate->inputs[i].parent = gate;
    gate->inputs[i].connections = NULL;
  }

  for (int i = 0; i < numOutputs; i++)
  {
    gate->outputs[i].xOffset = 0;
    gate->outputs[i].yOffset = 0;
    gate->outputs[i].isInput = false;
    gate->outputs[i].parent = gate;
    gate->outputs[i].connections = NULL;
  }
}
void InitIOGate(LogicElement *gate, int id, int ioMode, int numOfPins)
{
  int xPos = 0;
  if (ioMode == 0)
  {
    xPos = 45;
  }
  else
  {
    xPos = x_size - 45;
  }

  int yStart = 45;
  int yEnd = y_size - 45;

  int span = (yEnd - yStart) / numOfPins;

  int yPos = id * span + span / 2 + yStart;

  gate->active = true;
  gate->id = id;
  gate->x = xPos;
  gate->y = yPos;

  int numInputs = 0;
  int numOutputs = 0;
  if (ioMode == 0)
  {
    numInputs = 0;
    numOutputs = 1;

    gate->currentState = false;
    gate->nextState = false;
  }
  else
  {
    numInputs = 1;
    numOutputs = 0;
  }

  gate->numInputs = numInputs;
  gate->numOutputs = numOutputs;
  gate->inputs = malloc(numInputs * sizeof(io));
  gate->outputs = malloc(numOutputs * sizeof(io));

  memset(gate->name, 0, MAX_NAME_LENGTH);
  strncpy(gate->name, "", MAX_NAME_LENGTH - 1);

  gate->hitbox.x = xPos - IO_PIN_SIZE;
  gate->hitbox.y = yPos - IO_PIN_SIZE;
  gate->hitbox.width = 2 * IO_PIN_SIZE;
  gate->hitbox.height = 2 * IO_PIN_SIZE;
  gate->hitbox.onClick = ToggleIO;
  gate->hitbox.indexParam = id;
  gate->hitbox.triggerable = PRESS;
  gate->hitbox.freeable = false;

  for (int i = 0; i < numInputs; i++)
  {
    gate->inputs[i].xOffset = 0;
    gate->inputs[i].yOffset = 0;
    gate->inputs[i].isInput = true;
    gate->inputs[i].parent = gate;
    gate->inputs[i].connections = NULL;
    gate->inputs[i].numConnections = 0;

    gate->inputs->ioHitbox.x = xPos - IO_PIN_SIZE;
    gate->inputs->ioHitbox.y = yPos - IO_PIN_SIZE;
    gate->inputs->ioHitbox.width = 2 * IO_PIN_SIZE;
    gate->inputs->ioHitbox.height = 2 * IO_PIN_SIZE;
    gate->inputs->ioHitbox.onClick = DummyFunction;
    gate->inputs->ioHitbox.indexParam = -1;
    gate->inputs->ioHitbox.triggerable = DRAG;
    gate->inputs->ioHitbox.freeable = false;
    gate->outputs->ioHitbox.active = true;
  }

  for (int i = 0; i < numOutputs; i++)
  {
    gate->outputs[i].xOffset = 0;
    gate->outputs[i].yOffset = 0;
    gate->outputs[i].isInput = false;
    gate->outputs[i].parent = gate;
    gate->outputs[i].connections = NULL;
    gate->outputs[i].numConnections = 0;

    gate->outputs->ioHitbox.x = xPos - IO_PIN_SIZE;
    gate->outputs->ioHitbox.y = yPos - IO_PIN_SIZE;
    gate->outputs->ioHitbox.width = 2 * IO_PIN_SIZE;
    gate->outputs->ioHitbox.height = 2 * IO_PIN_SIZE;
    gate->outputs->ioHitbox.onClick = DummyFunction;
    gate->outputs->ioHitbox.indexParam = -1;
    gate->outputs->ioHitbox.triggerable = DRAG;
    gate->outputs->ioHitbox.freeable = false;
    gate->outputs->ioHitbox.active = true;
  }
}

void InitBasicLogicElements()
{
  definedElementsList = (LogicElement *)malloc(sizeof(LogicElement) * MAX_ELEMENTS);
  uiElementsList = (UILogicElement *)malloc(sizeof(UILogicElement) * MAX_ELEMENTS);
  otherButtonsList = (LogicElement *)malloc(sizeof(LogicElement) * MAX_ELEMENTS);
  placedElementsList = (LogicElement *)malloc(sizeof(LogicElement) * MAX_ELEMENTS);
  inputPinList = (LogicElement *)malloc(sizeof(LogicElement) * MAX_ELEMENTS);
  outputPinList = (LogicElement *)malloc(sizeof(LogicElement) * MAX_ELEMENTS);
  memset(definedElementsList, 0, sizeof(LogicElement) * MAX_ELEMENTS);
  memset(uiElementsList, 0, sizeof(UILogicElement) * MAX_ELEMENTS);
  memset(otherButtonsList, 0, sizeof(LogicElement) * MAX_ELEMENTS);
  memset(placedElementsList, 0, sizeof(LogicElement) * MAX_ELEMENTS);
  memset(inputPinList, 0, sizeof(LogicElement) * MAX_ELEMENTS);
  memset(outputPinList, 0, sizeof(LogicElement) * MAX_ELEMENTS);
  // init logic elements
  if (definedElementsList == NULL)
  {
    printf("MALLOC FAILED\n");
    return;
  }
  InitGate(&definedElementsList[0], 0, "IN", 0, 0, 0, 1, logic_or);
  InitGate(&definedElementsList[1], 1, "OUT", 0, 0, 1, 0, logic_or);
  InitGate(&definedElementsList[2], 2, "AND", 0, 0, 2, 1, logic_and);
  InitGate(&definedElementsList[3], 3, "OR", 0, 0, 2, 1, logic_or);
  InitGate(&definedElementsList[4], 4, "NOT", 0, 0, 1, 1, logic_not);
  InitGate(&definedElementsList[5], 5, "XOR", 0, 0, 2, 1, logic_xor);
  InitGate(&definedElementsList[6], 6, "NAND", 0, 0, 2, 1, logic_nand);
  InitGate(&definedElementsList[7], 7, "NOR", 0, 0, 2, 1, logic_nor);
  InitGate(&definedElementsList[8], 8, "XNOR", 0, 0, 2, 1, logic_xnor);
  InitGate(&definedElementsList[9], 9, "XAND", 0, 0, 2, 1, logic_xand);

  // init ui logic elements
  int uiIndex = 0;
  for (int i = 0; i < MAX_ELEMENTS; i++)
  {
    if (definedElementsList[i].active == false)
      continue;
    if (definedElementsList[i].numInputs == 1 && definedElementsList[i].numOutputs == 0 ||
        definedElementsList[i].numInputs == 0 && definedElementsList[i].numOutputs == 1 ||
        definedElementsList[i].id == 0 || definedElementsList[i].id == 1)
      continue;
    uiElementsList[uiIndex].active = true;
    uiElementsList[uiIndex].element = &definedElementsList[i];
    memset(uiElementsList[uiIndex].name, 0, MAX_NAME_LENGTH);
    strncpy(uiElementsList[uiIndex].name, definedElementsList[i].name, MAX_NAME_LENGTH - 1);
    uiElementsList[uiIndex].id = definedElementsList[i].id;
    uiIndex++;
  }
  selectedeElement = uiElementsList[0].id;
}

void InitIoPins()
{
  for (int i = 0; i < numInputPins; i++)
  {
    InitIOGate(&inputPinList[i], i, 0, numInputPins);
    if (ioMode == 1) // meaning in hardware mode -> bind inputs to real pins
    {
      // todo
    }
  }
  for (int i = 0; i < numOutputPins; i++)
  {
    InitIOGate(&outputPinList[i], i, 1, numOutputPins);
    if (ioMode == 1) // meaning in hardware mode -> bind outputs to leds
    {
      // todo
    }
  }
}

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC3_Init(void);
void DMA_Init(void);
void StartDefaultTask(void *argument);
void StartScreenTask(void *argument);
void StartTouchTask(void *argument);
void StartSimTask(void *argument);
static void CPU_CACHE_Enable(void);
void SetLed(int ledIndex, bool ledValue);

// void LTDC_IRQHandler(void)
// {
//   if (__HAL_LTDC_GET_FLAG(&hlcd_ltdc, LTDC_FLAG_RR) != RESET)
//   {
//     __HAL_LTDC_CLEAR_FLAG(&hlcd_ltdc, LTDC_FLAG_RR);
//     reloadDone = true;
//   }
// }

// void EnableLTDCReloadInterrupt()
// {
//   __HAL_LTDC_ENABLE_IT(&hlcd_ltdc, LTDC_IT_RR);
//   HAL_NVIC_SetPriority(LTDC_IRQn, 0, 0);
//   HAL_NVIC_EnableIRQ(LTDC_IRQn);
// }

// __attribute__((section(".RAM_D1"))) uint32_t myBuffer[LCD_WIDTH * LCD_HEIGHT];

// __attribute__((section(".RAM_D1"))) uint16_t drawBuffer_top[FRAMEBUFFER_SIZE / 2];
// __attribute__((section(".RAM_D2"))) uint16_t drawBuffer_bottom[FRAMEBUFFER_SIZE / 2];

uint16_t *drawBuffer_top;
__attribute__((section(".RAM_D2"))) uint16_t drawBuffer_bottom[FRAMEBUFFER_SIZE / 2];

void FillBuffer(uint16_t color)
{
  size_t halfSize = (LCD_WIDTH * LCD_HEIGHT) / 2;
  for (size_t i = 0; i < halfSize; ++i)
  {
    drawBuffer_top[i] = color;
  }
  for (int y = 0; y < LCD_HEIGHT; ++y)
  {
    for (int x = 0; x < LCD_WIDTH; ++x)
    {
      drawBuffer_bottom[y * LCD_WIDTH + x] = color;
    }
  }
}

void FillBufferTEST()
{
  for (int y = 0; y < LCD_HEIGHT; ++y)
  {
    for (int x = 0; x < LCD_WIDTH; ++x)
    {
      uint8_t r = (x * 255) / LCD_WIDTH;
      uint8_t g = (y * 255) / LCD_HEIGHT;
      uint8_t b = 128;
      drawBuffer_top[y * LCD_WIDTH + x] = RGB565(b, g, r);
      drawBuffer_bottom[y * LCD_WIDTH + x] = RGB565(r, g, b);
      // myBuffer_bottom[y * LCD_WIDTH + x] = ARGB8888(0xFF, r, g, b);
    }
  }
}

static inline void SetPixel(int x, int y, uint16_t color)
{
  if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT)
    return;

  if (y < LCD_HEIGHT / 2)
  {
    drawBuffer_top[y * LCD_WIDTH + x] = color;
  }
  else
  {
    int ylocal = y - LCD_HEIGHT / 2;
    drawBuffer_bottom[ylocal * LCD_WIDTH + x] = color;
  }
}

void DrawRect(int x, int y, int width, int height, uint16_t color)
{
  for (int dy = 0; dy < height; ++dy)
  {
    for (int dx = 0; dx < width; ++dx)
    {
      SetPixel(x + dx, y + dy, color);
    }
  }
}

void DrawCircle(int cx, int cy, int radius, uint16_t color)
{
  for (int y = -radius; y <= radius; y++)
  {
    for (int x = -radius; x <= radius; x++)
    {
      if (x * x + y * y <= radius * radius)
      {
        SetPixel(cx + x, cy + y, color);
      }
    }
  }
}

void DrawLine(int x0, int y0, int x1, int y1, int thickness, uint16_t color)
{
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;

  while (1)
  {
    for (int tx = -thickness / 2; tx <= thickness / 2; tx++)
    {
      for (int ty = -thickness / 2; ty <= thickness / 2; ty++)
      {
        SetPixel(x0 + tx, y0 + ty, color);
      }
    }

    if (x0 == x1 && y0 == y1)
      break;
    e2 = 2 * err;
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

void DrawChar(int x, int y, char c, sFONT *font, uint16_t color)
{
  if (c < ' ' || c > '~')
    return;

  int bytes_per_row = (font->Width + 7) / 8;
  int char_index = (c - ' ') * font->Height * bytes_per_row;

  for (int row = 0; row < font->Height; row++)
  {
    for (int col = 0; col < font->Width; col++)
    {
      int byte_index = char_index + row * bytes_per_row + (col / 8);
      uint8_t byte = font->table[byte_index];
      if (byte & (1 << (7 - (col % 8))))
      {
        SetPixel(x + col, y + row, color);
      }
    }
  }
}

void DrawText(int x, int y, const char *str, sFONT *font, uint16_t color)
{
  int cursorX = x - 1;
  y += font->Height;

  while (*str)
  {
    DrawChar(cursorX, y, *str, font, color);
    cursorX += font->Width + 1;
    str++;
  }
}

void TestText(int offset)
{
  DrawText(20 + offset % LCD_WIDTH, 30, "AbCdE -+_123.<>", &Font16, RGB565(0, 20, 0)); // White
}

void TestShapes()
{
  uint16_t red = 0xF800;
  uint16_t green = 0x07E0;
  uint16_t blue = 0x001F;

  DrawRect(50, 50, 100, 50, red);
  DrawCircle(200, 120, 30, green);
  DrawLine(20, 20, 400, 200, 3, blue);
}

void AnimateCircleRight(int offset, uint16_t color, int radius)
{
  int y = LCD_HEIGHT / 2;
  int x = offset % (LCD_WIDTH + radius * 2);
  x -= radius;

  if (x >= -radius && x <= LCD_WIDTH + radius)
  {
    DrawCircle(x, y, radius, color);
  }
}

void AnimateRectLeft(int offset, uint16_t color, int sideA, int sideB)
{
  int y = (LCD_HEIGHT / 2) - (sideB / 2);
  int x = LCD_WIDTH - (offset % (LCD_WIDTH + sideA));
  if (x + sideA >= 0 && x <= LCD_WIDTH)
  {
    DrawRect(x, y, sideA, sideB, color);
  }
}

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  CPU_CACHE_Enable();
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  // MX_GPIO_Init();
  // MX_ADC3_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();
  InitInputInterruptPins();
  // BSP_LED_Init(LED_GREEN);
  // BSP_LED_Init(LED_RED);

  // BSP_LED_Off(LED_GREEN);
  InitLeds();
  // all leds off
  SetLed(0, false);
  SetLed(1, false);
  SetLed(2, false);

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);

  /* Configure the User push-button in EXTI Mode */
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Configure TIM3 timebase */
  Init_TIM3(&TIM3Handle);

  /* Init UART3*/
  if (USART3_Init(&UART3Handle) != HAL_OK)
  {
    Error_Handler();
  }
  RetargetInit(&UART3Handle);

  // init dma
  DMA_Init();

  x_size = 0;
  y_size = 0;
  TS_State_display = (TS_State_t){0};
  hTS_display = (TS_Init_t){0};

  BSP_LCD_InitEx(0, LCD_ORIENTATION_LANDSCAPE, LCD_PIXEL_FORMAT_RGB565, LCD_DEFAULT_WIDTH, LCD_DEFAULT_HEIGHT);
  UTIL_LCD_SetFuncDriver(&LCD_Driver);

  BSP_LCD_GetXSize(0, &x_size);
  BSP_LCD_GetYSize(0, &y_size);

  uint16_t *fb = (uint16_t *)BSP_LCD_GetFramebufferAddress(0);

  InitBasicLogicElements();
  InitIoPins();

  drawBuffer_top = (uint16_t *)malloc(FRAMEBUFFER_SIZE / 2 * sizeof(uint16_t));
  // drawBuffer_bottom = (uint16_t *)malloc(FRAMEBUFFER_SIZE / 2 * sizeof(uint16_t));

  drawBufferMutex = xSemaphoreCreateMutex();
  if (drawBufferMutex == NULL)
  {
    Error_Handler();
  }

  displayStateMutex = xSemaphoreCreateMutex();
  if (displayStateMutex == NULL)
  {
    Error_Handler();
  }

  DrawToScreen();

  // DrawLoop();

  osKernelInitialize();

  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  touchTaskHandle = osThreadNew(StartTouchTask, NULL, &touchTask_attributes);
  screenTaskHandle = osThreadNew(StartScreenTask, NULL, &screenTask_attributes);
  simTaskHandle = osThreadNew(StartSimTask, NULL, &simTask_attributes);

  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief  Check for user input
 * @param  None
 * @retval Input state (1 : active / 0 : Inactive)
 */
uint8_t CheckForUserInput(void)
{
  return ButtonState;
}
/**
 * @brief  EXTI line detection callbacks.
 * @param  GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 */
void BSP_PB_Callback(Button_TypeDef Button)
{
  if (Button == BUTTON_USER)
  {
    ButtonState = 1;
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (ioMode)
  {
    // if (timeoutMs > 0)
    //   return;
    if (GPIO_Pin == GPIO_PIN_3) // D2 = PG3
    {
      GPIO_PinState state = HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_3);
      bool boolState = (state == GPIO_PIN_SET);
      inputPinList[0].currentState = boolState;
      inputPinList[0].nextState = boolState;
    }
    else if (GPIO_Pin == GPIO_PIN_6) // D3 = PA6
    {
      GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
      bool boolState = (state == GPIO_PIN_SET);
      inputPinList[1].currentState = boolState;
      inputPinList[1].nextState = boolState;
    }
    else if (GPIO_Pin == GPIO_PIN_8) // D7 = PI8
    {
      GPIO_PinState state = HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_8);
      bool boolState = (state == GPIO_PIN_SET);
      inputPinList[2].currentState = boolState;
      inputPinList[2].nextState = boolState;
    }
    timeoutMs = 50;
    needsRedraw = true;
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

  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
  {
  }

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
  if (ret != HAL_OK)
  {
    Error_Handler();
  }

  /* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 |
                                 RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if (ret != HAL_OK)
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
  __HAL_RCC_CSI_ENABLE();

  /* Enable SYSCFG clock mondatory for I/O Compensation Cell */
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  /* Enables the I/O Compensation Cell */
  HAL_EnableCompensationCell();
}

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

void DMA_Init(void)
{
  __HAL_RCC_DMA2_CLK_ENABLE();

  hdmaDisplay.Instance = DMA2_Stream0;
  hdmaDisplay.Init.Request = DMA_REQUEST_MEM2MEM;
  hdmaDisplay.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdmaDisplay.Init.PeriphInc = DMA_PINC_ENABLE;
  hdmaDisplay.Init.MemInc = DMA_MINC_ENABLE;
  hdmaDisplay.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdmaDisplay.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdmaDisplay.Init.Mode = DMA_NORMAL;
  hdmaDisplay.Init.Priority = DMA_PRIORITY_HIGH;
  hdmaDisplay.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdmaDisplay.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  hdmaDisplay.Init.MemBurst = DMA_MBURST_INC4;
  hdmaDisplay.Init.PeriphBurst = DMA_PBURST_INC4;

  if (HAL_DMA_Init(&hdmaDisplay) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief ADC3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */

  /** Common config
   */
  hadc3.Instance = ADC3;
  hadc3.Init.Resolution = ADC_RESOLUTION_16B;
  hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc3.Init.LowPowerAutoWait = DISABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  hadc3.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc3.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc3.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
   */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.SingleDiff = ADC_DIFFERENTIAL_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOK_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PI6 PI5 PI4 PI7 */
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_SAI2;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pins : FDCAN2_RX_Pin FDCAN2_TX_Pin */
  GPIO_InitStruct.Pin = FDCAN2_RX_Pin | FDCAN2_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PK5 PK4 PK6 PK3
                           PK7 PK2 */
  GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_3 | GPIO_PIN_7 | GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
  HAL_GPIO_Init(GPIOK, &GPIO_InitStruct);

  /*Configure GPIO pin : PG10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_SAI2;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : SDIO1_D2_Pin SDIO1_D3_Pin SDIO1_CK_Pin SDIO1_D0_Pin
                           SDIO1_D1_Pin SDIO1_D7_Pin SDIO1_D6_Pin */
  GPIO_InitStruct.Pin = SDIO1_D2_Pin | SDIO1_D3_Pin | SDIO1_CK_Pin | SDIO1_D0_Pin | SDIO1_D1_Pin | SDIO1_D7_Pin | SDIO1_D6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PI1 PI0 PI9 PI12
                           PI14 PI15 */
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_9 | GPIO_PIN_12 | GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pins : PE1 PE0 PE10 PE9
                           PE11 PE12 PE15 PE8
                           PE13 PE7 PE14 */
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_10 | GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15 | GPIO_PIN_8 | GPIO_PIN_13 | GPIO_PIN_7 | GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF7_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : MII_TX_EN_Pin MII_TXD1_Pin MII_TXD0_Pin */
  GPIO_InitStruct.Pin = MII_TX_EN_Pin | MII_TXD1_Pin | MII_TXD0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_DISP_Pin PJ14 PJ12 PJ13
                           PJ11 PJ10 PJ9 PJ0
                           PJ8 PJ7 PJ6 PJ1
                           PJ5 PJ3 PJ4 */
  GPIO_InitStruct.Pin = LCD_DISP_Pin | GPIO_PIN_14 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_11 | GPIO_PIN_10 | GPIO_PIN_9 | GPIO_PIN_0 | GPIO_PIN_8 | GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_1 | GPIO_PIN_5 | GPIO_PIN_3 | GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
  HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

  /*Configure GPIO pin : PD3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PI2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : PH15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pins : FDCAN1_RX_Pin FDCAN1_TX_Pin */
  GPIO_InitStruct.Pin = FDCAN1_RX_Pin | FDCAN1_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pin : MII_TXD3_Pin */
  GPIO_InitStruct.Pin = MII_TXD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(MII_TXD3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_DISPD7_Pin */
  GPIO_InitStruct.Pin = LCD_DISPD7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LCD_DISPD7_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PE5 PE4 */
  GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_SAI4;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : SDIO1_D5_Pin SDIO1_D4_Pin */
  GPIO_InitStruct.Pin = SDIO1_D5_Pin | SDIO1_D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PG15 PG8 PG5 PG4
                           PG0 PG1 */
  GPIO_InitStruct.Pin = GPIO_PIN_15 | GPIO_PIN_8 | GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : SDIO1_CMD_Pin */
  GPIO_InitStruct.Pin = SDIO1_CMD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
  HAL_GPIO_Init(SDIO1_CMD_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PD0 PD1 PD15 PD14
                           PD10 PD9 PD8 */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_15 | GPIO_PIN_14 | GPIO_PIN_10 | GPIO_PIN_9 | GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OTG_FS2_ID_Pin */
  GPIO_InitStruct.Pin = USB_OTG_FS2_ID_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OTG_FS2_ID_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_OTG_FS2_P_Pin USB_OTG_FS2_N_Pin */
  GPIO_InitStruct.Pin = USB_OTG_FS2_P_Pin | USB_OTG_FS2_N_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : MII_RX_ER_Pin */
  GPIO_InitStruct.Pin = MII_RX_ER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(MII_RX_ER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PF2 PF1 PF0 PF3
                           PF5 PF4 PF13 PF14
                           PF12 PF15 PF11 */
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_12 | GPIO_PIN_15 | GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_INT_Pin */
  GPIO_InitStruct.Pin = LCD_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LCD_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_BL_Pin */
  GPIO_InitStruct.Pin = LCD_BL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LCD_BL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PF6 PF7 PF10 */
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PC0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PF9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : MII_MDC_Pin MII_TXD2_Pin MII_TX_CLK_Pin MII_RXD0_Pin
                           MII_RXD1_Pin */
  GPIO_InitStruct.Pin = MII_MDC_Pin | MII_TXD2_Pin | MII_TX_CLK_Pin | MII_RXD0_Pin | MII_RXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : MII_CRS_Pin MII_COL_Pin */
  GPIO_InitStruct.Pin = MII_CRS_Pin | MII_COL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pins : MII_MDIO_Pin MII_RX_CLK_Pin MII_RX_DV_Pin */
  GPIO_InitStruct.Pin = MII_MDIO_Pin | MII_RX_CLK_Pin | MII_RX_DV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PH5 PH6 PH7 */
  GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pins : VCP_TX_Pin VCP_RX_Pin */
  GPIO_InitStruct.Pin = VCP_TX_Pin | VCP_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS2_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS2_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS2_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF9_TIM13;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PH9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pin : PD11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : MII_RX_D3_Pin MII_RX_D2_Pin */
  GPIO_InitStruct.Pin = MII_RX_D3_Pin | MII_RX_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : LD1_Pin */
  GPIO_InitStruct.Pin = LD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_RST_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*AnalogSwitch Config */
  HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PA0, SYSCFG_SWITCH_PA0_OPEN);

  /*AnalogSwitch Config */
  HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PA1, SYSCFG_SWITCH_PA1_OPEN);
}

void InitLeds(void)
{
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  // PD3
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  // PJ2
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

  // PI13
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
}
/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

void DrawToScreen();

// HELPERS

void AddConnection(io *a, io *b)
{
  // Check if connection already exists
  for (int i = 0; i < a->numConnections; ++i)
  {
    if (a->connections[i] == b)
    {
      RemoveConnection(a, b); // Remove existing connection before adding again
      return;
    }
  }

  // add b to a
  a->connections = realloc(a->connections, sizeof(io *) * (a->numConnections + 1));
  a->connections[a->numConnections++] = b;

  // add a to b
  b->connections = realloc(b->connections, sizeof(io *) * (b->numConnections + 1));
  b->connections[b->numConnections++] = a;
}

void RemoveOne(io *node, io *target)
{
  if (node == NULL || node->connections == NULL || node->numConnections == 0)
    return;

  int i, j;
  for (i = 0; i < node->numConnections; ++i)
  {
    if (node->connections[i] == target)
    {
      // shift remaining
      for (j = i; j < node->numConnections - 1; ++j)
      {
        node->connections[j] = node->connections[j + 1];
      }
      node->numConnections--;
      if (node->numConnections == 0)
      {
        free(node->connections);
        node->connections = NULL;
      }
      else
      {
        node->connections = realloc(node->connections, sizeof(io *) * node->numConnections);
      }
      break;
    }
  }
}

void RemoveConnection(io *a, io *b)
{
  if (a == NULL || b == NULL)
    return;

  RemoveOne(a, b);
  RemoveOne(b, a);
}
void DrawCenteredText(int centerX, int centerY, const char *text)
{
  sFONT *font = &Font16;

  int charWidth = font->Width;
  int charHeight = font->Height;
  int textWidth = strlen(text) * charWidth;
  int textHeight = charHeight;

  int startX = centerX - (textWidth / 2);
  int startY = centerY - (textHeight / 2);

  // UTIL_LCD_DisplayStringAt(startX, startY, (uint8_t *)text, LEFT_MODE);
  DrawText(startX, startY, text, font, RGB565(0, 0, 0));
}
void DrawCenteredTextColored(int centerX, int centerY, const char *text, int rgb565color)
{
  sFONT *font = &Font16;

  int charWidth = font->Width;
  int charHeight = font->Height;
  int textWidth = strlen(text) * charWidth;
  int textHeight = charHeight;

  int startX = centerX - (textWidth / 2);
  int startY = centerY - (textHeight / 2);

  DrawText(startX, startY, text, font, rgb565color);
}
void LeftArrowClick()
{
  currentTopBarPage--;
  if (currentTopBarPage < 0)
    currentTopBarPage = 0;
  DrawToScreen();
}
void RightArrowClick()
{
  currentTopBarPage++;
  if (currentTopBarPage > MAX_ELEMENTS)
    currentTopBarPage = MAX_ELEMENTS;
  DrawToScreen();
}
void AutoIncSim()
{
  // // todo
  // SetLed(0, simulating);
  // SetLed(1, simulating);
  // SetLed(2, simulating);

  simulating = !simulating;
  if (simulating)
  {
    HAL_TIM_Base_Start_IT(&htim2);
  }
  else
  {
    HAL_TIM_Base_Stop_IT(&htim2);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
  }
}
void UIelementClick(int index)
{
  // TODO if u click same element multiple times, it adds inputs -> click AND 2x -> 3 inputs
  selectedeElement = uiElementsList[index].id;
  uiSelectedeElement = index;
  DrawToScreen();
}
void IOminus(int index)
{
  if (ioMode == 1)
    return;
  if (index == 0)
  {
    numInputPins--;
    if (numInputPins < 1)
    {
      numInputPins = 1;
    }
  }
  else
  {
    numOutputPins--;
    if (numOutputPins < 1)
    {
      numOutputPins = 1;
    }
  }
  memset(inputPinList, 0, sizeof(LogicElement) * MAX_ELEMENTS);
  memset(outputPinList, 0, sizeof(LogicElement) * MAX_ELEMENTS);
  InitIoPins();
  DrawToScreen();
}
void IOplus(int index)
{
  if (ioMode == 1)
    return;
  if (index == 0)
  {
    numInputPins++;
    if (numInputPins > MAX_IO)
    {
      numInputPins = MAX_IO;
    }
  }
  else
  {
    numOutputPins++;
    if (numOutputPins > MAX_IO)
    {
      numOutputPins = MAX_IO;
    }
  }
  memset(inputPinList, 0, sizeof(LogicElement) * MAX_ELEMENTS);
  memset(outputPinList, 0, sizeof(LogicElement) * MAX_ELEMENTS);
  InitIoPins();
  DrawToScreen();
}
void SwitchOutMode()
{
  ioMode = !ioMode;
  if (ioMode == 1)
  {
    numInputPins = 3;
    numOutputPins = 3;
    InitIoPins();
  }
  DrawToScreen();
}
void CreateHitbox(LogicElement *element, int x, int y, int width, int height, enum HitBoxTriggerable triggerable, bool freeable, void (*onClick)(int), int indexParam)
{
  element->x = x;
  element->y = y;
  element->hitbox.x = x;
  element->hitbox.y = y;
  element->hitbox.width = width;
  element->hitbox.height = height;
  element->hitbox.onClick = onClick;
  element->hitbox.indexParam = indexParam;
  element->hitbox.triggerable = triggerable;
  element->hitbox.freeable = freeable;
}
void CreateIoHitbox(io *element, int ioCenterX, int ioCenterY, int radius, void (*onClick)(int), int indexParam)
{
  element->ioHitbox.x = ioCenterX - radius;
  element->ioHitbox.y = ioCenterY - radius;
  element->ioHitbox.width = radius * 2;
  element->ioHitbox.height = radius * 2;
  element->ioHitbox.onClick = onClick;
  element->ioHitbox.indexParam = indexParam;
  element->ioHitbox.triggerable = DRAG;
  element->ioHitbox.freeable = false;
  element->ioHitbox.active = true;
}
bool IsPointInHitbox(HitBox *hitbox, int px, int py)
{
  return (
      px >= hitbox->x &&
      px <= hitbox->x + hitbox->width &&
      py >= hitbox->y &&
      py <= hitbox->y + hitbox->height);
}
io *CheckIoHitboxes(int x, int y, enum HitBoxTriggerable triggerable)
{
  for (int i = numInputPins - 1; i >= 0; i--)
  {
    for (int j = 0; j < inputPinList[i].numOutputs; j++)
    {
      if (!IsPointInHitbox(&inputPinList[i].outputs[j].ioHitbox, x, y))
        continue;
      if (inputPinList[i].outputs[j].ioHitbox.triggerable != triggerable)
        continue;
      return &inputPinList[i].outputs[j];
    }
  }

  for (int i = numOutputPins - 1; i >= 0; i--)
  {
    for (int j = 0; j < outputPinList[i].numInputs; j++)
    {
      if (!IsPointInHitbox(&outputPinList[i].inputs[j].ioHitbox, x, y))
        continue;
      if (outputPinList[i].inputs[j].ioHitbox.triggerable != triggerable)
        continue;
      return &outputPinList[i].inputs[j];
    }
  }

  for (int i = MAX_ELEMENTS - 1; i >= 0; i--)
  {
    if (placedElementsList[i].active == false)
      continue;
    for (int j = 0; j < placedElementsList[i].numInputs; j++)
    {
      if (!IsPointInHitbox(&placedElementsList[i].inputs[j].ioHitbox, x, y))
        continue;
      if (placedElementsList[i].inputs[j].ioHitbox.triggerable != triggerable)
        continue;
      return &placedElementsList[i].inputs[j];
    }
    for (int j = 0; j < placedElementsList[i].numOutputs; j++)
    {
      if (!IsPointInHitbox(&placedElementsList[i].outputs[j].ioHitbox, x, y))
        continue;
      if (placedElementsList[i].outputs[j].ioHitbox.triggerable != triggerable)
        continue;
      return &placedElementsList[i].outputs[j];
    }
  }
  return NULL;
}
HitBox *CheckHitboxes(int x, int y, enum HitBoxTriggerable triggerable)
{
  for (int i = numInputPins; i >= 0; i--)
  {
    if (inputPinList[i].active == false)
      continue;
    if (!IsPointInHitbox(&inputPinList[i].hitbox, x, y))
      continue;
    if (inputPinList[i].hitbox.triggerable != triggerable)
      continue;
    inputPinList[i].hitbox.onClick(inputPinList[i].hitbox.indexParam);
    return &inputPinList[i].hitbox;
  }

  for (int i = MAX_ELEMENTS - 1; i >= 0; i--)
  {
    if (otherButtonsList[i].active == false)
      continue;
    if (!IsPointInHitbox(&otherButtonsList[i].hitbox, x, y))
      continue;
    if (otherButtonsList[i].hitbox.triggerable != triggerable)
      continue;
    otherButtonsList[i].hitbox.onClick(otherButtonsList[i].hitbox.indexParam);
    return &otherButtonsList[i].hitbox;
  }

  for (int i = MAX_ELEMENTS - 1; i >= 0; i--)
  {
    if (uiElementsList[i].active == false)
      continue;
    if (!IsPointInHitbox(&uiElementsList[i].element->hitbox, x, y))
      continue;
    if (uiElementsList[i].element->hitbox.triggerable != triggerable)
      continue;
    uiElementsList[i].element->hitbox.onClick(uiElementsList[i].element->hitbox.indexParam);
    return &uiElementsList[i].element->hitbox;
  }

  for (int i = MAX_ELEMENTS - 1; i >= 0; i--)
  {
    if (placedElementsList[i].active == false)
      continue;
    if (!IsPointInHitbox(&placedElementsList[i].hitbox, x, y))
      continue;
    if (placedElementsList[i].hitbox.triggerable != triggerable)
      continue;
    placedElementsList[i].hitbox.onClick(placedElementsList[i].hitbox.indexParam);
    return &placedElementsList[i].hitbox;
  }
  return NULL;
}
int CalculateSpacing(int minY, int maxY, int numOfPoints)
{
  if (numOfPoints <= 1)
    return 0;

  int totalDistance = maxY - minY;
  return totalDistance / (numOfPoints - 1);
}
void SetLed(int ledIndex, bool ledValue)
{
  ledValue = !ledValue;
  if (ledIndex == 0)
  {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, !ledValue);
  }
  else if (ledIndex == 1)
  {
    HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, ledValue);
  }
  else
  {
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, ledValue);
  }
}
// HELPERS END

void draw(LogicElement *element)
{
  int x = element->x;
  int y = element->y;

  uint16_t clr;
  DrawRect(x, y, logicElementWidth, logicElementHeight, SECONDARYFOREGROUND);
  DrawCenteredTextColored(x + logicElementWidth / 2, y + logicElementHeight / 4, (const char *)element->name, PRIMARYFOREGROUND);
  for (int i = 0; i < element->numInputs; i++)
  {
    io input = element->inputs[i];

    bool inputState = false;
    for (int k = 0; k < input.numConnections; k++)
    {
      if (k == 0)
        inputState = input.connections[k]->parent->currentState;
      else
        inputState |= input.connections[k]->parent->currentState;
    }

    if (inputState == true)
      clr = ONCOLOR;
    else
      clr = OFFCOLOR;
    DrawCircle(x + input.xOffset, y + input.yOffset, IO_CIRCLE_SIZE, clr);
  }
  for (int i = 0; i < element->numOutputs; i++)
  {
    io output = element->outputs[i];
    if (element->currentState == true)
      clr = ONCOLOR;
    else
      clr = OFFCOLOR;
    DrawCircle(x + output.xOffset, y + output.yOffset, IO_CIRCLE_SIZE, clr);
  }
}

void drawIO(LogicElement *element)
{
  int x = element->x;
  int y = element->y;

  uint16_t clr;
  DrawCircle(x, y, IO_PIN_SIZE, SECONDARYFOREGROUND);
  for (int i = 0; i < element->numInputs; i++)
  {
    io input = element->inputs[i];

    bool inputState = false;
    for (int k = 0; k < input.numConnections; k++)
    {
      if (k == 0)
        inputState = input.connections[k]->parent->currentState;
      else
        inputState |= input.connections[k]->parent->currentState;
    }
    element->currentState = inputState;
    if (inputState == true)
      clr = ONCOLOR;
    else
      clr = OFFCOLOR;
    DrawCircle(x + input.xOffset, y + input.yOffset, IO_CIRCLE_SIZE, clr);
  }
  for (int i = 0; i < element->numOutputs; i++)
  {
    if (element->currentState == true)
      clr = ONCOLOR;
    else
      clr = OFFCOLOR;
    io output = element->outputs[i];
    DrawCircle(x + output.xOffset, y + output.yOffset, IO_CIRCLE_SIZE, clr);
  }
}

void DrawLines(LogicElement *element, uint16_t onColor, uint16_t offColor, int thickness)
{
  // for each input get its connections and draw between connection and input
  // for each output get its connections and draw between connection and output
  uint16_t color;
  for (int i = 0; i < element->numInputs; i++)
  {
    io input = element->inputs[i];

    for (int j = 0; j < input.numConnections; j++)
    {
      io *connection = input.connections[j];

      if (connection->parent->currentState)
      {
        color = onColor;
      }
      else
      {
        color = offColor;
      }
      DrawLine(input.xOffset + input.parent->x,
               input.yOffset + input.parent->y,
               connection->xOffset + connection->parent->x,
               connection->yOffset + connection->parent->y, thickness, color);

      // UTIL_LCD_DrawLine(input.xOffset + input.parent->x,
      //                   input.yOffset + input.parent->y,
      //                   connection->xOffset + connection->parent->x,
      //                   connection->yOffset + connection->parent->y, color);
    }
  }
  for (int i = 0; i < element->numOutputs; i++)
  {
    io output = element->outputs[i];

    for (int j = 0; j < output.numConnections; j++)
    {
      io *connection = output.connections[j];

      if (element->currentState)
      {
        color = onColor;
      }
      else
      {
        color = offColor;
      }
      DrawLine(output.xOffset + output.parent->x,
               output.yOffset + output.parent->y,
               connection->xOffset + connection->parent->x,
               connection->yOffset + connection->parent->y, thickness, color);

      // UTIL_LCD_DrawLine(output.xOffset + output.parent->x,
      //                   output.yOffset + output.parent->y,
      //                   connection->xOffset + connection->parent->x,
      //                   connection->yOffset + connection->parent->y, color);
    }
  }
  // line drawing
  // UTIL_LCD_DrawLine(startIo->xOffset + startIo->parent->x,
  //   startIo->yOffset + startIo->parent->y,
  //   endIo->xOffset + endIo->parent->x,
  //   endIo->yOffset + endIo->parent->y,
  //   UTIL_LCD_COLOR_BLACK);
}

void DrawLogicElements()
{
  for (int i = 0; i < numInputPins; i++)
  {
    drawIO(&inputPinList[i]);
  }
  for (int i = 0; i < numOutputPins; i++)
  {
    drawIO(&outputPinList[i]);
  }
  for (int i = 0; i < MAX_ELEMENTS; i++)
  {
    if (placedElementsList[i].active == true)
    {
      draw(&placedElementsList[i]);
      DrawLines(&placedElementsList[i], ONCOLOR, OFFCOLOR, 3);
    }
  }
}

void simulateElement(LogicElement *element)
{
  // step 1: get inputs
  bool *inputStates = malloc(element->numInputs * sizeof(bool));
  for (int j = 0; j < element->numInputs; j++)
  {
    inputStates[j] = false; // default to false
    io *conn = &element->inputs[j];
    // 1 for each input get "OR" of all connected
    for (int k = 0; k < conn->numConnections; k++)
    {
      if (k == 0)
        inputStates[j] = conn->connections[k]->parent->currentState;
      else
        inputStates[j] |= conn->connections[k]->parent->currentState;
    }
  }
  // step 2: use apropriate funct
  // todo : temp function (and)
  LogicFunction clbck = element->logicCallback;
  element->nextState = clbck(inputStates, element->numInputs);

  // bool resultingState = inputStates[0];
  // for (int j = 1; j < element->numInputs; j++)
  // {
  //   inputStates[0] &= inputStates[j];
  // }
}

void SimulateStep1()
{
  // 1: get inputs, 2: use apropriate funct, 3: set nextState
  for (int i = 0; i < MAX_ELEMENTS; i++)
  {
    if (placedElementsList[i].active == false)
      continue;
    simulateElement(&placedElementsList[i]);
  }
}
void SimulateStep2()
{
  for (int i = 0; i < MAX_ELEMENTS; i++)
  {
    if (placedElementsList[i].active == false)
      continue;
    placedElementsList[i].currentState = placedElementsList[i].nextState;
  }
}

void ToggleSim()
{
}

void SimulateTick()
{
  SimulateStep1();
  SimulateStep2();
}

void CreateButton(LogicElement *element, int x, int y, int width, int height, char *text, void (*onClick)(int), int indexParam, enum HitBoxTriggerable triggerable, bool freeable, uint16_t color, uint16_t textColor)
{
  DrawRect(x, y, width, height, color);
  DrawCenteredTextColored(x + (width / 2), y, text, textColor);
  element->active = true;
  CreateHitbox(element, x, y, width, height, triggerable, freeable, onClick, indexParam);
}

void DrawUi()
{
  // UTIL_LCD_SetFont(&Font16);
  // UTIL_LCD_SetTextColor(SECONDARYCOLOR);
  // UTIL_LCD_SetBackColor(PRIMARYCOLOR);

  int buttonWidth = 50;
  int buttonHeight = 30;
  int margin = 5;
  int maxPerPage = 7;

  int allButtonOffset = margin + buttonWidth / 2 + margin + 14;

  // draw top bar bg
  DrawRect(0, 0, x_size, buttonHeight + 2 * margin, SECONDARYFOREGROUND);

  // draw arrows
  // left
  CreateButton(&otherButtonsList[0], margin, 5, buttonWidth / 2, buttonHeight, "<-", LeftArrowClick, -1, PRESS, true, PRIMARYFOREGROUND, SECONDARYFOREGROUND);

  // right
  CreateButton(&otherButtonsList[1], x_size - margin - buttonWidth / 2, 5, buttonWidth / 2, buttonHeight, "->", RightArrowClick, -1, PRESS, true, PRIMARYFOREGROUND, SECONDARYFOREGROUND);

  // draw top bar elements (leftArrow - element - element ... - element - rightArrow)
  for (int i = 0; i < MAX_ELEMENTS; i++)
  {
    uiElementsList[i].active = false;
  }
  int start = maxPerPage * currentTopBarPage;
  for (int i = start; i < start + maxPerPage; i++)
  {
    if (uiElementsList[i].element == NULL)
      continue;
    if (uiElementsList[i].element->active == false)
      continue;
    uiElementsList[i].active = true;
    if (uiSelectedeElement == i)
    {
      CreateButton(uiElementsList[i].element, (i % maxPerPage) * (margin + buttonWidth) + allButtonOffset, 5, buttonWidth, buttonHeight, uiElementsList[i].name, UIelementClick, i, PRESS, true, PRIMARYCOLOR, PRIMARYFOREGROUND);
    }
    else
    {
      CreateButton(uiElementsList[i].element, (i % maxPerPage) * (margin + buttonWidth) + allButtonOffset, 5, buttonWidth, buttonHeight, uiElementsList[i].name, UIelementClick, i, PRESS, true, PRIMARYFOREGROUND, SECONDARYFOREGROUND);
    }
  }

  // draw bottom buttons
  DrawText(margin + 1, y_size - 5 - buttonHeight - 10 - 8, "(long)press: O/I", &Font8, SECONDARYFOREGROUND);

  // press
  CreateButton(&otherButtonsList[2], margin, y_size - 5 - buttonHeight, buttonHeight, buttonHeight, "-", IOminus, 1, LONGPRESS, true, SECONDARYFOREGROUND, PRIMARYFOREGROUND);
  CreateButton(&otherButtonsList[3], margin + 30 + 2, y_size - 5 - buttonHeight, buttonWidth, buttonHeight, "IO:+", IOplus, 1, LONGPRESS, true, SECONDARYFOREGROUND, PRIMARYFOREGROUND);

  // long press
  CreateButton(&otherButtonsList[4], margin, y_size - 5 - buttonHeight, buttonHeight, buttonHeight, "-", IOminus, 0, PRESS, true, SECONDARYFOREGROUND, PRIMARYFOREGROUND);
  CreateButton(&otherButtonsList[5], margin + 30 + 2, y_size - 5 - buttonHeight, buttonWidth, buttonHeight, "IO:+", IOplus, 0, PRESS, true, SECONDARYFOREGROUND, PRIMARYFOREGROUND);
  // mode switch
  if (ioMode == true)
  {
    // hardware
    CreateButton(&otherButtonsList[6], x_size - margin - buttonWidth * 2, y_size - 5 - buttonHeight, buttonWidth * 2, buttonHeight, "HW CTRL", SwitchOutMode, -1, PRESS, true, PRIMARYCOLOR, PRIMARYFOREGROUND);

    // button text
    DrawText(12, 40, "PIN:", &Font8, SECONDARYFOREGROUND);
    DrawText(15, 64, "D2", &Font8, SECONDARYFOREGROUND);
    DrawText(15, 124, "D3", &Font8, SECONDARYFOREGROUND);
    DrawText(15, 184, "D7", &Font8, SECONDARYFOREGROUND);
  }
  else
  {
    // software
    CreateButton(&otherButtonsList[6], x_size - margin - buttonWidth * 2, y_size - 5 - buttonHeight, buttonWidth * 2, buttonHeight, "SW CTRL", SwitchOutMode, -1, PRESS, true, SECONDARYFOREGROUND, PRIMARYFOREGROUND);
  }
  CreateButton(&otherButtonsList[7], x_size - margin - buttonWidth * 2 - margin - buttonWidth * 0.75, y_size - 5 - buttonHeight, buttonWidth * 0.75, buttonHeight, "->", SimulateTick, -1, PRESS, true, SECONDARYFOREGROUND, PRIMARYFOREGROUND);
  // if (simulating == true)
  //   CreateButton(&otherButtonsList[8], x_size - margin - buttonWidth * 2 - margin - buttonWidth, y_size - 5 - buttonHeight, buttonWidth, buttonHeight, "->", ToggleSim, -1, PRESS, true, PRIMARYCOLOR);
  // else
  //   CreateButton(&otherButtonsList[8], x_size - margin - buttonWidth * 2 - margin - buttonWidth, y_size - 5 - buttonHeight, buttonWidth, buttonHeight, "||", ToggleSim, -1, PRESS, true, PRIMARYCOLOR);

  DrawText(x_size - margin - buttonWidth * 2 - margin - buttonWidth * 0.75, y_size - 5 - buttonHeight - 10 - 8, "longpress: autoInc", &Font8, SECONDARYFOREGROUND);

  if (simulating)
    CreateButton(&otherButtonsList[8], x_size - margin - buttonWidth * 2 - margin - buttonWidth * 0.75, y_size - 5 - buttonHeight, buttonWidth * 0.75, buttonHeight, "->", AutoIncSim, -1, LONGPRESS, true, PRIMARYCOLOR, PRIMARYFOREGROUND);
  else
    CreateButton(&otherButtonsList[8], x_size - margin - buttonWidth * 2 - margin - buttonWidth * 0.75, y_size - 5 - buttonHeight, buttonWidth * 0.75, buttonHeight, "->", AutoIncSim, -1, LONGPRESS, true, SECONDARYFOREGROUND, PRIMARYFOREGROUND);
}

void DummyFunction(int indexParam)
{
  return;
}
void ToggleIO(int indexParam)
{
  inputPinList[indexParam].currentState = !inputPinList[indexParam].currentState;
  inputPinList[indexParam].nextState = !inputPinList[indexParam].nextState;
}

void DestroyElement(int indexParam)
{
  for (int i = placedElementsList[indexParam].numInputs - 1; i >= 0; i--)
  {
    for (int j = placedElementsList[indexParam].inputs[i].numConnections - 1; j >= 0; j--)
    {
      io *ioA = &placedElementsList[indexParam].inputs[i];
      io *ioB = placedElementsList[indexParam].inputs[i].connections[j];
      RemoveConnection(ioA, ioB);
    }
  }
  for (int i = placedElementsList[indexParam].numOutputs - 1; i >= 0; i--)
  {
    for (int j = placedElementsList[indexParam].outputs[i].numConnections - 1; j >= 0; j--)
    {
      RemoveConnection(&placedElementsList[indexParam].outputs[i], placedElementsList[indexParam].outputs[i].connections[j]);
    }
  }
  placedElementsList[indexParam].active = false;
}
// creates a logic element at x,y
void PlaceLogicElement(int x, int y)
{
  int xCorner = x - logicElementWidth / 2;
  int yCorner = y - logicElementHeight / 2;
  for (int i = 0; i < MAX_ELEMENTS; i++)
  {
    if (placedElementsList[i].active == false)
    {
      placedElementsList[i].id = definedElementsList[selectedeElement].id;
      strncpy(placedElementsList[i].name, definedElementsList[selectedeElement].name, MAX_NAME_LENGTH);
      memset(&placedElementsList[i].hitbox, 0, sizeof(HitBox));

      placedElementsList[i].x = xCorner;
      placedElementsList[i].y = yCorner;
      placedElementsList[i].active = true;
      placedElementsList[i].currentState = 0;
      placedElementsList[i].nextState = 0;

      // placedElementsList[i].numInputs = 2;  // TODO TEMP
      // placedElementsList[i].numOutputs = 1; // TODO TEMP

      placedElementsList[i].logicCallback = definedElementsList[selectedeElement].logicCallback;

      placedElementsList[i].numInputs = definedElementsList[selectedeElement].numInputs;
      placedElementsList[i].numOutputs = definedElementsList[selectedeElement].numOutputs;

      placedElementsList[i].inputs = malloc(sizeof(io) * placedElementsList[i].numInputs);
      placedElementsList[i].outputs = malloc(sizeof(io) * placedElementsList[i].numOutputs);

      int padding = 1.5 * IO_CIRCLE_SIZE;
      int inputSpacing = CalculateSpacing(-logicElementHeight / 2 + padding, logicElementHeight / 2 - padding, placedElementsList[i].numInputs);
      for (int j = 0; j < placedElementsList[i].numInputs; j++)
      {
        placedElementsList[i].inputs[j].parent = &placedElementsList[i];
        placedElementsList[i].inputs[j].isInput = true;
        placedElementsList[i].inputs[j].numConnections = 0;
        placedElementsList[i].inputs[j].connections = NULL;
        placedElementsList[i].inputs[j].xOffset = 0;
        placedElementsList[i].inputs[j].yOffset = padding + (j * inputSpacing);
        CreateIoHitbox(&placedElementsList[i].inputs[j],
                       xCorner + placedElementsList[i].inputs[j].xOffset,
                       yCorner + placedElementsList[i].inputs[j].yOffset,
                       IO_CIRCLE_SIZE, DummyFunction, 0);
      }
      int outputSpacing = CalculateSpacing(-logicElementHeight / 2, logicElementHeight / 2, placedElementsList[i].numOutputs);
      for (int j = 0; j < placedElementsList[i].numOutputs; j++)
      {
        placedElementsList[i].outputs[j].parent = &placedElementsList[i];
        placedElementsList[i].outputs[j].isInput = false;
        placedElementsList[i].outputs[j].numConnections = 0;
        placedElementsList[i].outputs[j].connections = NULL;
        placedElementsList[i].outputs[j].xOffset = logicElementWidth;
        placedElementsList[i].outputs[j].yOffset = padding + (j * outputSpacing);
        CreateIoHitbox(&placedElementsList[i].outputs[j],
                       xCorner + placedElementsList[i].outputs[j].xOffset,
                       yCorner + placedElementsList[i].outputs[j].yOffset,
                       IO_CIRCLE_SIZE, DummyFunction, 0);
      }
      CreateHitbox(&placedElementsList[i], xCorner, yCorner, logicElementWidth, logicElementHeight, LONGPRESS, false, DestroyElement, i);
      return;
    }
  }
}

void DrawToScreen()
{
  while (true)
    if (xSemaphoreTake(drawBufferMutex, pdMS_TO_TICKS(1)) == pdTRUE)
    {
      if (definedElementsList == NULL)
        return;

      FillBuffer(RGB565(255, 255, 255)); // clear buffer

      DrawUi();
      DrawLogicElements();

      xSemaphoreGive(drawBufferMutex);
      break;
    }
}

void UpdateOutLeds()
{
  if (ioMode == 1)
  {
    for (int i = 0; i < numOutputPins; i++)
    {
      SetLed(i, outputPinList[i].currentState);
    }
  }
}

void UpdateInPins()
{
}

// basic + io manager task
void StartDefaultTask(void *argument)
{
  InitIoPins();
  for (;;)
  {
    // todo io pins
    UpdateOutLeds();
    vTaskDelay(pdMS_TO_TICKS(16));
    timeoutMs -= 16;
  }
}
void StartSimTask(void *argument)
{
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    SimulateTick();
    // toggl = !toggl;
    // SetLed(0, toggl);

    DrawToScreen();
  }
}

bool IsWithinLongHoldTreshold()
{
  return (abs(holdStartX - holdEndX) <= 3 && abs(holdStartY - holdEndY) <= 3);
}

void NormalPressLogic()
{
  HitBox *hitHitbox = CheckHitboxes(holdEndX, holdEndY, PRESS);
  if (hitHitbox == NULL)
  {
    // place element
    PlaceLogicElement(holdEndX, holdEndY);
  }
  DrawToScreen();
}

void LongPressLogic()
{
  CheckHitboxes(holdEndX, holdEndY, LONGPRESS);
  DrawToScreen();
}

void DragLogic()
{
  // if start withing hitbox with drag type and end within hitbox drag type -> first draw line -> later implement io struct connection
  io *startIo = CheckIoHitboxes(holdStartX, holdStartY, DRAG);
  io *endIo = CheckIoHitboxes(holdEndX, holdEndY, DRAG);
  if (!(startIo && endIo))
    return;

  // HitBox startHitbox = startIo->ioHitbox;
  // HitBox endHitbox = endIo->ioHitbox;
  // draw line
  // UTIL_LCD_DrawLine(startIo->xOffset + startIo->parent->x,
  //                   startIo->yOffset + startIo->parent->y,
  //                   endIo->xOffset + endIo->parent->x,
  //                   endIo->yOffset + endIo->parent->y,
  //                   UTIL_LCD_COLOR_BLACK);

  AddConnection(startIo, endIo);
  DrawToScreen();
}

void StartTouchTask(void *argument)
{
  int ts_status = BSP_ERROR_NONE;

  /* touchscreen init */
  hTS_display.Width = x_size;
  hTS_display.Height = y_size;
  hTS_display.Orientation = TS_SWAP_XY;
  hTS_display.Accuracy = 5;
  ts_status = BSP_TS_Init(0, &hTS_display);

  if (ts_status != BSP_ERROR_NONE)
  {
    return;
  }

  for (;;)
  {
    ts_status = BSP_TS_GetState(0, &TS_State_display);

    if (TS_State_display.TouchDetected &&
        TS_State_display.TouchX < x_size &&
        TS_State_display.TouchY < y_size)
    {
      if (!isHolding)
      {
        isHolding = true;
        holdStartTimeStamp = HAL_GetTick();

        holdStartX = TS_State_display.TouchX;
        holdStartY = TS_State_display.TouchY;
        holdEndX = TS_State_display.TouchX;
        holdEndY = TS_State_display.TouchY;
      }
      else
      {
        holdEndX = TS_State_display.TouchX;
        holdEndY = TS_State_display.TouchY;
      }
    }
    else if (isHolding)
    {
      isHolding = false;
      if (IsWithinLongHoldTreshold())
      {
        if (HAL_GetTick() - holdStartTimeStamp > 250)
        {
          LongPressLogic();
        }
        else
        {
          NormalPressLogic();
        }
      }
      else
      {
        DragLogic();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

uint32_t ARGB8888(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
  return (a << 24) | (r << 16) | (g << 8) | b;
}

uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void CopyBuffersToDisplay_DMA(uint16_t *fb)
{
  // top half (in RAM_D1)
  SCB_CleanDCache_by_Addr((uint32_t *)drawBuffer_top, oneBufferSize);
  HAL_DMA_Start(&hdmaDisplay, (uint32_t)(drawBuffer_top), (uint32_t)(fb), oneBufferSize / sizeof(uint16_t));
  while (HAL_DMA_PollForTransfer(&hdmaDisplay, HAL_DMA_FULL_TRANSFER, 100) != HAL_OK)
  {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  SCB_InvalidateDCache_by_Addr((uint32_t *)fb, oneBufferSize);

  // bot half (in RAM_D2)
  SCB_CleanDCache_by_Addr((uint32_t *)drawBuffer_bottom, oneBufferSize);
  HAL_DMA_Start(&hdmaDisplay, (uint32_t)(drawBuffer_bottom), (uint32_t)(fb) + (oneBufferSize), oneBufferSize / sizeof(uint16_t));
  while (HAL_DMA_PollForTransfer(&hdmaDisplay, HAL_DMA_FULL_TRANSFER, 100) != HAL_OK)
  {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  SCB_InvalidateDCache_by_Addr((uint32_t *)(fb + (oneBufferSize / sizeof(uint16_t))), oneBufferSize);
}
void StartScreenTask(void *argument)
{
  screenTaskHandle = xTaskGetCurrentTaskHandle();
  // EnableLTDCReloadInterrupt();

  // int offst = 0;

  for (;;)
  {
    // wait for a notification from the timer
    if (needsRedraw)
    {
      needsRedraw = false;
      DrawToScreen();
    }
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (xSemaphoreTake(drawBufferMutex, pdMS_TO_TICKS(1)) == pdTRUE)
    {
      // copy buffers to the display buffer
      // memcpy((void *)fb, drawBuffer_top, FRAMEBUFFER_SIZE / 2);
      // memcpy((void *)fb + FRAMEBUFFER_SIZE / 2, drawBuffer_bottom, FRAMEBUFFER_SIZE / 2);

      // copy buffers to the display buffer with DMA
      CopyBuffersToDisplay_DMA(fb);
      SCB_CleanDCache_by_Addr((uint32_t *)fb, FRAMEBUFFER_SIZE);

      xSemaphoreGive(drawBufferMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void MX_TIM2_Init(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE();

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = (SystemCoreClock / 1000) - 1; // becomes 64000-1
  htim2.Init.Period = 5000 - 1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  HAL_TIM_Base_Init(&htim2);

  HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0); // Priority compatible with FreeRTOS
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

void MX_TIM6_Init(void)
{
  __HAL_RCC_TIM6_CLK_ENABLE();

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = (SystemCoreClock / 1000) - 1;
  htim6.Init.Period = 16 - 1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  HAL_TIM_Base_Init(&htim6);
  HAL_TIM_Base_Start_IT(&htim6);

  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

void InitInputInterruptPins(void)
{
  // Enable GPIO and SYSCFG clocks
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;

  // D2 = PG3
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  // D7 = PI8
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  // D3 = PA6
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
  // HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void TIM6_DAC_IRQHandler(void)
{
  if (__HAL_TIM_GET_FLAG(&htim6, TIM_FLAG_UPDATE) != RESET)
  {
    if (__HAL_TIM_GET_IT_SOURCE(&htim6, TIM_IT_UPDATE) != RESET)
    {
      __HAL_TIM_CLEAR_IT(&htim6, TIM_IT_UPDATE);

      BaseType_t xHigherPriorityTaskWoken = pdFALSE;

      if (screenTaskHandle != NULL)
      {
        vTaskNotifyGiveFromISR(screenTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
    }
  }
}

void TIM2_IRQHandler(void)
{
  if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) != RESET)
  {
    if (__HAL_TIM_GET_IT_SOURCE(&htim2, TIM_IT_UPDATE) != RESET)
    {
      __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);

      BaseType_t xHigherPriorityTaskWoken = pdFALSE;

      if (simTaskHandle != NULL)
      {
        vTaskNotifyGiveFromISR(simTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
    }
  }
}

// int ts_status = BSP_ERROR_NONE;

/* Touchscreen initialization */
// hTS_display.Width = x_size;
// hTS_display.Height = y_size;
// hTS_display.Orientation = TS_SWAP_XY;
// hTS_display.Accuracy = 5;
// ts_status = BSP_TS_Init(0, &hTS_display);

// if (ts_status != BSP_ERROR_NONE)
// {
//   return;
// }
// DrawToScreen();
// }

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  FillBuffer(RGB565(255, 0, 0));
  memcpy((void *)fb, drawBuffer_top, FRAMEBUFFER_SIZE / 2);
  memcpy((void *)fb + FRAMEBUFFER_SIZE / 2, drawBuffer_bottom, FRAMEBUFFER_SIZE / 2);
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
