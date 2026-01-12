#include "main.h"
#include <stdbool.h>
void display_main_screen(void);
void handle_main_screen_touch(uint16_t x, uint16_t y);
bool game_in_progress;
void display_main_screen(void) {
	game_in_progress = false;
    UTIL_LCD_Clear(UTIL_LCD_COLOR_WHITE);

    // Define three game areas with different colors
    UTIL_LCD_FillRect(0, 0, 160, 272, UTIL_LCD_COLOR_DARKBLUE);
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_DARKBLUE);
    UTIL_LCD_SetFont(&Font16);
    UTIL_LCD_DisplayStringAt(8000, 128, (uint8_t *)"Memory game", CENTER_MODE);

    UTIL_LCD_FillRect(160, 0, 160, 272, UTIL_LCD_COLOR_ST_BLUE);
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_ST_BLUE);
    UTIL_LCD_SetFont(&Font16);
    UTIL_LCD_DisplayStringAt(480, 143, (uint8_t *)"Tic Tac Toe", CENTER_MODE);

    UTIL_LCD_FillRect(320, 0, 160, 272, UTIL_LCD_COLOR_DARKBLUE);
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_DARKBLUE);
    UTIL_LCD_SetFont(&Font16);
    UTIL_LCD_DisplayStringAt(4000, 136, (uint8_t *)"Whack-a-Mole", CENTER_MODE);
}

void handle_main_screen_touch(uint16_t x, uint16_t y) {
	if (current_state != MAIN_SCREEN) return;

    if (x < 160) {
    	if (!game_in_progress){
    		current_state = MEMORY_GAME;
    		game_in_progress = true;
    		start_memory_game();
    	}
    } else if (x < 320) {
    	if (!game_in_progress){
    		current_state = TIC_TAC_TOE;
			game_in_progress = true;
			start_tic_tac_toe();
    	}
    } else {
    	UTIL_LCD_FillRect(320, 0, 160, 272, UTIL_LCD_COLOR_ST_BLUE);
    	if (!game_in_progress){
    		current_state = WHACK_A_MOLE;
			game_in_progress = true;
    		start_whack_a_mole();
    	}
    }
}
