#include "main.h"
#include <stdbool.h>
#include <stdlib.h>

#define MAX_SEQUENCE_LENGTH 100
#define COLOR_COUNT 4
#define INITIAL_SEQUENCE_LENGTH 3
#define COLOR_ON_TIME 500   // Color stays on for 500ms
#define COLOR_OFF_TIME 500  // Gap between colors

typedef enum { RED, BLUE, GREEN, YELLOW } Color;

Color color_sequence[MAX_SEQUENCE_LENGTH];
int sequence_length = INITIAL_SEQUENCE_LENGTH;
int player_position = 0;
int memory_score = 0;
bool game_started = false;

// Coordinates for the squares (left-aligned)
const uint16_t square_positions[COLOR_COUNT][2] = {
    {50, 120},   // RED
    {150, 120},  // BLUE
    {250, 120},  // GREEN
    {350, 120}   // YELLOW
};
const uint16_t square_size = 80;  // Size of each square

void start_memory_game(void);
void draw_start_button(void);
void generate_sequence(void);
void play_sequence(void);
void handle_memory_game_touch(uint16_t x, uint16_t y);
void check_player_input(Color color);
void show_game_over(void);
void reset_memory_game(void);
void show_color(Color color);
void clear_color(Color color);
void draw_color_squares(void);
void show_message(const char* message);

void start_memory_game(void) {
    UTIL_LCD_Clear(UTIL_LCD_COLOR_DARKBLUE);
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_SetFont(&Font16);
    UTIL_LCD_DisplayStringAt(6, 25, (uint8_t *)"Back", LEFT_MODE);
    UTIL_LCD_DrawHLine(0, 60, 60, UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_DrawVLine(60, 0, 60, UTIL_LCD_COLOR_WHITE);
    draw_start_button();
    game_started = false;
}

void draw_start_button(void) {
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_SetFont(&Font16);
    UTIL_LCD_DisplayStringAt(180, 140, (uint8_t *)"Start game", LEFT_MODE);
    UTIL_LCD_DrawRect(140, 125, 200, 40, UTIL_LCD_COLOR_WHITE);
}

void draw_color_squares(void) {
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    for (int i = 0; i < COLOR_COUNT; i++) {
        UTIL_LCD_DrawRect(square_positions[i][0], square_positions[i][1], square_size, square_size, UTIL_LCD_COLOR_WHITE);
    }
}

void generate_sequence(void) {
    int previous_color = -1;
    int consecutive_count = 0;

    for (int i = 0; i < sequence_length; i++) {
        int new_color;
        do {
            new_color = rand() % COLOR_COUNT;
        } while (new_color == previous_color && consecutive_count >= 3);

        if (new_color == previous_color) {
            consecutive_count++;
        } else {
            consecutive_count = 1;
            previous_color = new_color;
        }
        color_sequence[i] = new_color;
    }
}

void play_sequence(void) {
	HAL_Delay(300);
    draw_color_squares();
    show_message("Watch closely!");

    for (int i = 0; i < sequence_length; i++) {
        show_color(color_sequence[i]);
        HAL_Delay(COLOR_ON_TIME);
        clear_color(color_sequence[i]);
        HAL_Delay(COLOR_OFF_TIME);
    }
    show_message("Begin!");
}

void show_message(const char* message) {
    // Clear the area where the message will be shown
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_DARKBLUE);
    UTIL_LCD_FillRect(0, 240, 480, 40, UTIL_LCD_COLOR_DARKBLUE);  // Adjust based on your screen dimensions

    // Display the new message
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_SetFont(&Font16);
    UTIL_LCD_DisplayStringAt(0, 240, (uint8_t *)message, CENTER_MODE);

    draw_color_squares();  // Redraw squares after showing the message
}

void handle_memory_game_touch(uint16_t x, uint16_t y) {
	if (current_state != MEMORY_GAME) return;

    if (x < 60 && y < 60) {
        reset_memory_game();  // Reset the game state before going back
        current_state = MAIN_SCREEN;
        display_main_screen();
        return;
    }

    if (!game_started) {
        if (x >= 140 && x <= 340 && y >= 125 && y <= 165) {
        	// Remove the start sequencce button from the screen
        	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_DARKBLUE);
        	UTIL_LCD_FillRect(140, 125, 200, 40, UTIL_LCD_COLOR_DARKBLUE);

            game_started = true;
            generate_sequence();
            play_sequence();
            player_position = 0;  // Reset player position for guessing
        }
    } else {
        for (int i = 0; i < COLOR_COUNT; i++) {
            uint16_t sx = square_positions[i][0];
            uint16_t sy = square_positions[i][1];
            if (x >= sx && x <= sx + square_size && y >= sy && y <= sy + square_size) {
                show_color((Color)i);   // Show the color for the player
                HAL_Delay(COLOR_ON_TIME);
                clear_color((Color)i);  // Clear the color after the delay
                check_player_input((Color)i);
                return;
            }
        }
    }
}

void check_player_input(Color color) {
    if (color == color_sequence[player_position]) {
        player_position++;
        if (player_position == sequence_length) {
            memory_score++;
            sequence_length++;
            HAL_Delay(500);
            show_message("Congratulations! Prepare for the next level!");
            HAL_Delay(1500);
            play_sequence();  // Start new sequence
            player_position = 0;
        }
    } else {
        show_game_over();
    }
}

void show_game_over(void) {
    game_started = false;
    char game_over_msg[50];
    snprintf(game_over_msg, sizeof(game_over_msg), "Better luck next time! Your score: %d", memory_score);
    show_message(game_over_msg);
    HAL_Delay(3000);
    reset_memory_game();
    current_state = MAIN_SCREEN;
    display_main_screen();
}

void reset_memory_game(void) {
    sequence_length = INITIAL_SEQUENCE_LENGTH;
    memory_score = 0;
    player_position = 0;
    game_started = false;
}

void show_color(Color color) {
    uint16_t sx = square_positions[color][0];
    uint16_t sy = square_positions[color][1];
    switch (color) {
        case RED:
            UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_RED);
            UTIL_LCD_FillRect(sx, sy, square_size, square_size, UTIL_LCD_COLOR_RED);
            break;
        case BLUE:
            UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_BLUE);
            UTIL_LCD_FillRect(sx, sy, square_size, square_size, UTIL_LCD_COLOR_BLUE);
            break;
        case GREEN:
            UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_GREEN);
            UTIL_LCD_FillRect(sx, sy, square_size, square_size, UTIL_LCD_COLOR_GREEN);
            break;
        case YELLOW:
            UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);
            UTIL_LCD_FillRect(sx, sy, square_size, square_size, UTIL_LCD_COLOR_YELLOW);
            break;
    }
}

void clear_color(Color color) {
    uint16_t sx = square_positions[color][0];
    uint16_t sy = square_positions[color][1];
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_DARKBLUE);
    UTIL_LCD_FillRect(sx, sy, square_size, square_size, UTIL_LCD_COLOR_DARKBLUE);
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_DrawRect(sx, sy, square_size, square_size, UTIL_LCD_COLOR_WHITE);
}
