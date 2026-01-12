#include "main.h"
#include <stdbool.h>

char grid[3][3];  // 3x3 grid for storing 'X', 'O', or empty
int current_player = 1;  // 1 = X, 2 = O
bool first_touch;

void draw_grid(void);
void check_game_status(void);

void start_tic_tac_toe(void) {
    // Clear the screen and set background
    UTIL_LCD_Clear(UTIL_LCD_COLOR_DARKBLUE);

    // Draw the grid
    draw_grid();

    // Initialize grid state (0: empty, 1: X, 2: O)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            grid[i][j] = 0;  // Use 0 to represent empty cells
        }
    }

    current_player = 1; // X starts
    // Small delay to avoid touch carryover
    HAL_Delay(200);
}

void draw_grid(void) {
    // Draw vertical and horizontal lines in white
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);

    // Vertical lines
    UTIL_LCD_DrawVLine(120, 16, 240, UTIL_LCD_COLOR_WHITE); // Between first and second column
    UTIL_LCD_DrawVLine(200, 16, 240, UTIL_LCD_COLOR_WHITE); // Between first and second column
    UTIL_LCD_DrawVLine(280, 16, 240, UTIL_LCD_COLOR_WHITE); // Between second and third column
    UTIL_LCD_DrawVLine(360, 16, 240, UTIL_LCD_COLOR_WHITE); // Between second and third column


    // Horizontal lines
    UTIL_LCD_DrawHLine(120, 16, 240, UTIL_LCD_COLOR_WHITE); // Between first and second row
    UTIL_LCD_DrawHLine(120, 96, 240, UTIL_LCD_COLOR_WHITE); // Between first and second row
    UTIL_LCD_DrawHLine(120, 176, 240, UTIL_LCD_COLOR_WHITE); // Between second and third row
    UTIL_LCD_DrawHLine(120, 256, 240, UTIL_LCD_COLOR_WHITE); // Between second and third row


    // Display the "Back" button
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_SetFont(&Font16);
    UTIL_LCD_DisplayStringAt(6, 25, (uint8_t *)"Back", LEFT_MODE);
    UTIL_LCD_DrawHLine(0, 60, 60, UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_DrawVLine(60, 0, 60, UTIL_LCD_COLOR_WHITE);

}
void draw_player_mark(int row, int col, int player) {
    // Calculate the X and Y positions for the mark based on the row and column
    uint16_t x_pos = 151 + col * 80;  // Adjust based on the grid's width
    uint16_t y_pos = 47 + row * 80;  // Adjust based on the grid's height

    if (player == 1) {
        // Draw 'X' for Player 1
        UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);  // Set color for 'X'
        UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_DARKBLUE); // Set background to black
        UTIL_LCD_SetFont(&Font24);
        UTIL_LCD_DisplayStringAt(x_pos, y_pos, (uint8_t *)"X", LEFT_MODE);
    } else {
        // Draw 'O' for Player 2
        UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE); // Set color for 'O'
        UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_DARKBLUE); // Set background to black
        UTIL_LCD_SetFont(&Font24);
        UTIL_LCD_DisplayStringAt(x_pos, y_pos, (uint8_t *)"O", LEFT_MODE);
    }
}

void handle_tic_tac_toe_touch(uint16_t x, uint16_t y) {


//    if (first_touch) {
//        first_touch = false;
//        return;
//    }

    if (current_state != TIC_TAC_TOE) return;


    // Check if the back button is pressed
    if (x < 60 && y < 60) {
    	current_state = MAIN_SCREEN;  // Go back to main screen state
    	display_main_screen();        // Reload the main screen
        return;
    }

    int row = -1, col = -1;

    // Map touch coordinates to grid cells (adjusted for actual grid layout)
    if (x > 120 && x < 360) {
        if (x < 200) col = 0;
        else if (x < 280) col = 1;
        else col = 2;
    }
    if (y > 16 && y < 256) {
        if (y < 96) row = 0;
        else if (y < 176) row = 1;
        else row = 2;
    }

    if (row != -1 && col != -1 && grid[row][col] == 0) {
        // Place current player's mark (1 for X, 2 for O)
        grid[row][col] = current_player;

        // Draw X or O on the screen
        draw_player_mark(row, col, current_player);

        // Check for win or draw
        check_game_status();

        // Switch players
        current_player = (current_player == 1) ? 2 : 1;
    }
}

void check_game_status(void) {
    // Check rows, columns, and diagonals for a winner
    for (int i = 0; i < 3; i++) {
        if (grid[i][0] == grid[i][1] && grid[i][1] == grid[i][2] && grid[i][0] != 0) {
            tic_tac_toe_game_over(grid[i][0]);
            return;
        }
        if (grid[0][i] == grid[1][i] && grid[1][i] == grid[2][i] && grid[0][i] != 0) {
            tic_tac_toe_game_over(grid[0][i]);
            return;
        }
    }
    if (grid[0][0] == grid[1][1] && grid[1][1] == grid[2][2] && grid[0][0] != 0) {
        tic_tac_toe_game_over(grid[0][0]);
        return;
    }
    if (grid[0][2] == grid[1][1] && grid[1][1] == grid[2][0] && grid[0][2] != 0) {
        tic_tac_toe_game_over(grid[0][2]);
        return;
    }

    // Check for a draw (no empty spaces)
    int draw = 1;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (grid[i][j] == 0) {  // Check for empty spaces (0 represents empty)
                draw = 0;
                break;
            }
        }
    }
    if (draw) {
        tic_tac_toe_game_over('D');  // 'D' represents a draw
    }
}

void tic_tac_toe_game_over(char winner) {
    UTIL_LCD_Clear(UTIL_LCD_COLOR_ST_BLUE);
    if (winner == 1) {
        UTIL_LCD_DisplayStringAt(0, 120, (uint8_t *)"X player won!", CENTER_MODE);
    } else if (winner == 2) {
        UTIL_LCD_DisplayStringAt(0, 120, (uint8_t *)"O player won!", CENTER_MODE);
    } else {
        UTIL_LCD_DisplayStringAt(0, 120, (uint8_t *)"It's a draw!", CENTER_MODE);
    }
    HAL_Delay(3000);  // Display message for 3 seconds
    current_state = MAIN_SCREEN;  // Go back to main screen state
    display_main_screen();        // Reload the main screen
}
