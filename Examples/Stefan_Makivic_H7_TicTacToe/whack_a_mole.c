#include "main.h"
#include <stdbool.h>
#include <stdlib.h>

#define MAX_MOLES 4
#define GRID_WIDTH 9
#define GRID_HEIGHT 6
#define CELL_SIZE 40

typedef struct {
    int x, y;            // Mole's grid position
    uint32_t appear_time; // Time mole appeared
    bool active;          // Is the mole currently active
} Mole;

Mole moles[MAX_MOLES];    // Array to store up to 4 moles
int lives = 3;
int score = 0;
uint32_t mole_appearance_time[MAX_MOLES];
uint32_t random_duration[MAX_MOLES];
bool game_running = true;

void start_whack_a_mole(void);
void spawn_mole(void);
void handle_whack_a_mole_touch(uint16_t x, uint16_t y);
void check_mole_timeout(void);
void game_over(void);
void whack_a_mole_game_loop(void);
void draw_mole_grid_outline(void);
void reset_game_state(void);

void start_whack_a_mole(void) {
    // Clear the screen
    UTIL_LCD_Clear(UTIL_LCD_COLOR_DARKBLUE);

    draw_score_and_lives();

    game_running = true;
    whack_a_mole_game_loop();
}
void clear_mole(int i) {
    // Clear the mole area only (draw a blue rectangle over the mole)
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_DARKBLUE);
    UTIL_LCD_FillCircle(moles[i].x + CELL_SIZE / 2, moles[i].y + CELL_SIZE / 2, 17, UTIL_LCD_COLOR_DARKBLUE);
}

void draw_score_and_lives(void){
    draw_mole_grid_outline();

    // Draw the back button
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_SetFont(&Font16);
    UTIL_LCD_DisplayStringAt(6, 25, (uint8_t *)"Back", LEFT_MODE);
    UTIL_LCD_DrawHLine(0, 60, 60, UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_DrawVLine(60, 0, 60, UTIL_LCD_COLOR_WHITE);
	// Display score
	char score_text[10];
	snprintf(score_text, sizeof(score_text), "Score: %d", score);
	UTIL_LCD_DisplayStringAt(190, 15, (uint8_t *)score_text, LEFT_MODE);

	// Display lives
	char lives_text[10];
	snprintf(lives_text, sizeof(lives_text), "Lives: %d", lives);
	UTIL_LCD_DisplayStringAt(380, 15, (uint8_t *)lives_text, LEFT_MODE);
}

void draw_mole_grid_outline(void) {
    // Draw only the white rectangle outline (80, 32) to (440, 270)
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_DrawHLine(80, 32, 360, UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_DrawHLine(80, 270, 360, UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_DrawVLine(80, 32, 239, UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_DrawVLine(440, 32, 239, UTIL_LCD_COLOR_WHITE);
}

void spawn_mole(void) {
	for(int i = 0; i < MAX_MOLES; i++){
		if (!moles[i].active) {
			    moles[i].x = 80 + (rand() % GRID_WIDTH) * CELL_SIZE;  // Random position in the grid (starting from 80, 32)
			    moles[i].y = 32 + (rand() % GRID_HEIGHT) * CELL_SIZE;
			    moles[i].appear_time = HAL_GetTick();  // Record the time the mole appeared
			    moles[i].active = true;

			    // Draw the mole on the screen
			    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BROWN);
			    UTIL_LCD_FillCircle(moles[i].x + CELL_SIZE / 2, moles[i].y + CELL_SIZE / 2, 17, UTIL_LCD_COLOR_BROWN);

			    mole_appearance_time[i] = HAL_GetTick();
			    random_duration[i] = rand() % 2000 + 3000;  // Mole appears for 2 to 50 seconds
			    return;
		}

	}

}

void handle_whack_a_mole_touch(uint16_t x, uint16_t y) {
    // Check if the back button is pressed
    if (current_state != WHACK_A_MOLE) return;

    if (x < 60 && y < 60) {
        reset_game_state();  // Reset the game state before going back
        current_state = MAIN_SCREEN;
        display_main_screen();
        return;
    }

    // Check if the player tapped on any mole
    for (int i = 0; i < MAX_MOLES; i++) {
        if (moles[i].active && x >= moles[i].x && x <= moles[i].x + CELL_SIZE && y >= moles[i].y && y <= moles[i].y + CELL_SIZE) {
            score++;
            moles[i].active = false;  // Remove the mole after being hit
            clear_mole(i);  // Clear the mole
            draw_score_and_lives();  // Redraw score and lives
        }
    }
}

void check_mole_timeout(void) {
    for (int i = 0; i < MAX_MOLES; i++) {
        if (moles[i].active && (HAL_GetTick() - mole_appearance_time[i] > random_duration[i])) {
            lives--;
            clear_mole(i);  // Clear only the mole area
            moles[i].active = false;  // Mole disappears after timeout
            draw_score_and_lives();  // Redraw score and lives
            if (lives <= 0) {
                game_over();
                break;
            }
        }
    }
}

void game_over(void) {
    game_running = false;  // Stop the game loop
    UTIL_LCD_Clear(UTIL_LCD_COLOR_ST_BLUE);
    char game_over_msg[50];
    snprintf(game_over_msg, sizeof(game_over_msg), "Nice try! Your score was: %d", score);
    UTIL_LCD_DisplayStringAt(0, 120, (uint8_t *)game_over_msg, CENTER_MODE);
    HAL_Delay(3000);  // Wait for 3 seconds before returning to the main screen
    reset_game_state();
    current_state = MAIN_SCREEN;
    display_main_screen();
}

void whack_a_mole_game_loop(void) {
    // Initialization or reset of game state
    reset_game_state();
    spawn_mole();  // Start by spawning the first mole
}

void reset_game_state(void) {
    // Reset all game variables for a new game
    lives = 3;
    score = 0;
    game_running = false;
    for (int i = 0; i < MAX_MOLES; i++) {
        moles[i].active = false;
    }
}
