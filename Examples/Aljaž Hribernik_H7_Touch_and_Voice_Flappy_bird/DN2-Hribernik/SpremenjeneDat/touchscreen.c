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
#define  CIRCLE_RADIUS        15
#define  LINE_LENGHT          30


#define SCREEN_W 480
#define SCREEN_H 272
#define FRAMEBUFFER_BASE  LCD_LAYER_0_ADDRESS

/* Private macro -------------------------------------------------------------*/

/* Private Structures and Enumerations ------------------------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
TS_State_t  TS_State;

int score;
int game_state = 0; // 0 = start
int game_speed;
int bird_pos_x;



static float bird_y;     // sub-pixel Y (center)
static float bird_vy;    // px/s
static uint32_t last_tick_ms;
static int prev_bird_y_px; // for erasing previous circle
static float dist_since_last_pipe = 0.0f;


static const float G_px_s2      = 490.0f;   // gravity
static const float V_FLAP_px_s  = -170.0f;  // upward impulse
static const float V_DOWN_MAX   = 195.0f;   // max fall speed
static const float V_UP_MAX     = -220.0f;  // cap rise speed
static const int PIPE_SPACING = 290;
static const int MAX_PIPES = 100;
static const int PIPE_W = 40;
static const int PIPE_GAP = 38;



typedef struct {
	int x;
	int y;
	int prev_x;
	int alive; // 0 = dead, 1 = alive
	int cleared; // 0 = bird not cleared the pipe yet, 1 = bird cleared the pipe already -> so
} Pipe;

Pipe pipes[100];
uint8_t pipe_count = 0;


/* Private variables ---------------------------------------------------------*/
/* Static variable holding the current touch color index : used to change color at each touch */
TS_Init_t hTS;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/


static void GameInit(void);
static void DrawScore(void);
static void DrawStartScreen(void);
static void DrawGameOverScreen(void);
static void DrawVictoryScreen(void);
static void DrawPipe(Pipe, int);
static void SpawnPipe(void);
static void UpdatePipes(float);
static inline int clampi(int v, int lo, int hi);
static void CollisionDetection(Pipe p);
static int CircleRectIntersect(int cx, int cy, int rad, int rx, int ry, int rw, int rh);

/**
  * @brief  Touchscreen Demo1 : test touchscreen calibration and single touch in polling mode
  * @param  None
  * @retval None
  */




void FlappyBird(void)
{

  uint32_t ts_status = BSP_ERROR_NONE;

  hTS.Width = SCREEN_W;
  hTS.Height = SCREEN_H;
  hTS.Orientation =TS_SWAP_XY ;
  hTS.Accuracy = 5;

  /* Touchscreen initialization */
  ts_status = BSP_TS_Init(0, &hTS);

  GameInit();

  if(ts_status == BSP_ERROR_NONE)
  {


	bird_pos_x = SCREEN_W/2;
	srand(time(NULL));
	DrawStartScreen();


    while (1)
    {
      /* Check in polling mode in touch screen the touch status and coordinates */
      /* of touches if touch occurred
	                                             */
      ts_status = BSP_TS_GetState(0, &TS_State);
      if(TS_State.TouchDetected)
      {
    	  if (game_state == 0) {
    		  game_state = 1; // starts game
    		  UTIL_LCD_Clear(UTIL_LCD_COLOR_LIGHTBLUE);
    		  UTIL_LCD_FillCircle(bird_pos_x, (int)bird_y, CIRCLE_RADIUS, UTIL_LCD_COLOR_YELLOW);
    	  }
    	  else if (game_state == 1) {
    		  bird_vy = V_FLAP_px_s;
    	  }
    	  else if (game_state == 2) {
    		  game_state = 0;
			  if (score < 100) DrawStartScreen();
			  else DrawVictoryScreen();
    		  GameInit();
    	  }

      }
      if (game_state == 1) { // update part

		if (ADC1->DR > 400) bird_vy = V_FLAP_px_s; // Preveri samo če je igra v teku

		// clear previous bird
		UTIL_LCD_FillCircle(bird_pos_x, prev_bird_y_px, CIRCLE_RADIUS, UTIL_LCD_COLOR_LIGHTBLUE);

		// Compute dt (seconds) since last update
		uint32_t now = HAL_GetTick();
		float dt = (now - last_tick_ms) * 0.001f;
		if (dt > 0.05f) dt = 0.05f; // safety clamp (50 ms)
		last_tick_ms = now;

		// Physics: gravity, clamp velocity, integrate position
		bird_vy += G_px_s2 * dt;
		if (bird_vy > V_DOWN_MAX) bird_vy = V_DOWN_MAX;
		if (bird_vy < V_UP_MAX)   bird_vy = V_UP_MAX;

		bird_y += bird_vy * dt;

		// convert to integer pixel for drawing
		int bird_y_px = (int)(bird_y + 0.5f);

		// bounds + game over
		if (bird_y_px - CIRCLE_RADIUS/2 < 0 || bird_y_px + CIRCLE_RADIUS/2 > SCREEN_H) {
			game_state = 2;
			// Optionally pin to edge so draw doesn’t under/overflow
			if (bird_y_px < CIRCLE_RADIUS/2) bird_y_px = CIRCLE_RADIUS/2;
			if (bird_y_px > SCREEN_H - CIRCLE_RADIUS/2) bird_y_px = SCREEN_H - CIRCLE_RADIUS/2;
		}


		UpdatePipes(dt);

		UTIL_LCD_FillCircle(bird_pos_x, bird_y_px, CIRCLE_RADIUS, UTIL_LCD_COLOR_YELLOW);
		DrawScore();

		prev_bird_y_px = bird_y_px;
      }

      HAL_Delay(40);
    }
  }

}


static void GameInit(void) {
	  score = 0;
	  memset(pipes, 0, MAX_PIPES);
	  pipe_count = 0;
	  game_speed = 40;
	  bird_vy = 0.0f;
	  bird_y  = SCREEN_H * 0.5f;
	  prev_bird_y_px = (int)bird_y;
	  last_tick_ms = HAL_GetTick();
}

static void DrawScore(void) {
    char buf[64];
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_LIGHTBLUE);
	UTIL_LCD_SetFont(&Font24);
	sprintf(buf, "%d", score);
	UTIL_LCD_DisplayStringAt(30, 30, (uint8_t *) buf, LEFT_MODE);
}

static void DrawGameOverScreen(void) {
	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
	UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_LIGHTBLUE);
	UTIL_LCD_SetFont(&Font24);
	UTIL_LCD_DisplayStringAt(0, SCREEN_H / 2, (uint8_t *)"GAME OVER", CENTER_MODE);
	UTIL_LCD_SetFont(&Font12);
	UTIL_LCD_DisplayStringAt(0, SCREEN_H / 2 + 30, (uint8_t *)"Press the anywhere to restart", CENTER_MODE);
}


static void DrawVictoryScreen(void) {
	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
	UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_LIGHTBLUE);
	UTIL_LCD_SetFont(&Font24);
	UTIL_LCD_DisplayStringAt(0, SCREEN_H / 2, (uint8_t *)"YOU WIN", CENTER_MODE);
	UTIL_LCD_SetFont(&Font12);
	UTIL_LCD_DisplayStringAt(0, SCREEN_H / 2 + 30, (uint8_t *)"Press the anywhere to restart", CENTER_MODE);
}

static void DrawStartScreen(void) {
	UTIL_LCD_Clear(UTIL_LCD_COLOR_LIGHTBLUE);
	UTIL_LCD_FillCircle(bird_pos_x, 100, CIRCLE_RADIUS, UTIL_LCD_COLOR_YELLOW);
	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
	UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_LIGHTBLUE);
	UTIL_LCD_SetFont(&Font24);
	UTIL_LCD_DisplayStringAt(0, SCREEN_H / 2, (uint8_t *)"Flappy Bird", CENTER_MODE);
	UTIL_LCD_SetFont(&Font12);
	UTIL_LCD_DisplayStringAt(0, SCREEN_H / 2 + 30, (uint8_t *)"Press the anywhere to start", CENTER_MODE);
}

static void SpawnPipe(void) {
	Pipe p = {SCREEN_W, rand() % (SCREEN_H - 50 - 50 + 1) + 50, SCREEN_W, 1};
	pipes[pipe_count++]= p;
}

static void DrawPipe(Pipe p, int dx) {
	uint16_t lower_pipe_y = p.y + PIPE_GAP;
	uint16_t upper_pipe_y = p.y - PIPE_GAP;


	if (p.prev_x + PIPE_W <= SCREEN_W) {
		// Clear previous pipe strip
		UTIL_LCD_FillRect(p.prev_x + PIPE_W - dx, 0, dx, upper_pipe_y, UTIL_LCD_COLOR_LIGHTBLUE);
		UTIL_LCD_FillRect(p.prev_x + PIPE_W - dx, lower_pipe_y, dx, SCREEN_H - lower_pipe_y, UTIL_LCD_COLOR_LIGHTBLUE);
	}
	if (p.x > 0) {
		UTIL_LCD_FillRect(p.x, 0, dx, upper_pipe_y, UTIL_LCD_COLOR_GREEN);
		UTIL_LCD_FillRect(p.x, lower_pipe_y, dx, SCREEN_H - lower_pipe_y, UTIL_LCD_COLOR_GREEN);
	}

}


static void UpdatePipes(float dt) {
	float dx_f = game_speed * dt;
	dist_since_last_pipe += dx_f;

	int dx = (int)dx_f;
	if (dx > 0) {
		for (int i = 0; i < pipe_count; i++){
			if (pipes[i].alive == 0) continue;
			pipes[i].x -= dx;

			if (pipes[i].cleared == 0) CollisionDetection(pipes[i]);

			DrawPipe(pipes[i], dx);
			pipes[i].prev_x = pipes[i].x;
			if (pipes[i].x + PIPE_W < 0) pipes[i].alive = 0;
			if (pipes[i].cleared == 0 && pipes[i].x + PIPE_W < bird_pos_x) {
				score++;
				pipes[i].cleared = 1;
			};
		}
	}

	while(pipe_count < MAX_PIPES && dist_since_last_pipe >= (float)PIPE_SPACING){
		SpawnPipe();
		dist_since_last_pipe -= (float)PIPE_SPACING;
	}
}


static void CollisionDetection(Pipe p) {
	uint16_t lower_pipe_y = p.y + PIPE_GAP;
	uint16_t upper_pipe_y = p.y - PIPE_GAP;
	int bird_y_px = (int)(bird_y + 0.5f);


	if (CircleRectIntersect(bird_pos_x, bird_y_px, CIRCLE_RADIUS, p.x, 0, PIPE_W, upper_pipe_y) == 1) {
		  game_state = 2;
		  DrawGameOverScreen();
	}
	if (CircleRectIntersect(bird_pos_x, bird_y_px, CIRCLE_RADIUS, p.x, lower_pipe_y, PIPE_W, SCREEN_H - lower_pipe_y) == 1) {
		  game_state = 2;
		  DrawGameOverScreen();
	}

}


static int CircleRectIntersect(int cx, int cy, int rad, int rx, int ry, int rw, int rh) {
    if (rw <= 0 || rh <= 0) return 0;

    // find closest point on rect to circle center
    int closest_x = clampi(cx, rx, rx + rw);
    int closest_y = clampi(cy, ry, ry + rh);

    // squared distance to that point
    int dx = cx - closest_x;
    int dy = cy - closest_y;
    int dist2 = dx*dx + dy*dy;

    return dist2 <= rad*rad;
}


static inline int clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}


