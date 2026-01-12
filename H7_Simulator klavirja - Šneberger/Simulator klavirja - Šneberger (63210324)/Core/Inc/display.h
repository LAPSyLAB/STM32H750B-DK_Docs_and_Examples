/*
 * display.h
 *
 *  Created on: Sep 14, 2025
 *      Author: nejcs
 */

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

#define LCD_WIDTH 		480
#define LCD_HEIGHT  	272

#define WHITE_KEY_W		(LCD_WIDTH / 7)
#define WHITE_KEY_H 	LCD_HEIGHT
#define BLACK_KEY_W		(WHITE_KEY_W * 2 / 3)
#define BLACK_KEY_H		(WHITE_KEY_H * 2 / 3)

typedef struct
{
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
	char* symbol;
	float frequency;
	int white;
	int pressed;
	int currentlyTouched;
	uint32_t index;
} Key;

extern Key keys[12];

#endif /* INC_DISPLAY_H_ */
