#include "main.h"
#include "stm32_lcd.h"
#include "display.h"

Key keys[] =
{
	{
		0, 0, WHITE_KEY_W, WHITE_KEY_H , "C", 261.63, 1, 0
	},
	{
		WHITE_KEY_W - BLACK_KEY_W/2, 0, BLACK_KEY_W, BLACK_KEY_H, "C#", 277.18, 0, 1
	},
	{
		WHITE_KEY_W, 0, WHITE_KEY_W, WHITE_KEY_H , "D", 293.66, 1, 2
	},
	{
		2 * WHITE_KEY_W - BLACK_KEY_W/2, 0, BLACK_KEY_W, BLACK_KEY_H, "D#", 311.13, 0, 3
	},
	{
		2 * WHITE_KEY_W, 0, WHITE_KEY_W, WHITE_KEY_H , "E", 329.63, 1, 4
	},
	{
		3 * WHITE_KEY_W, 0, WHITE_KEY_W, WHITE_KEY_H , "F", 349.23, 1, 5
	},
	{
		4 * WHITE_KEY_W - BLACK_KEY_W/2, 0, BLACK_KEY_W, BLACK_KEY_H, "F#", 369.99, 0, 6
	},
	{
		4 * WHITE_KEY_W, 0, WHITE_KEY_W, WHITE_KEY_H , "G", 392.00, 1, 7
	},
	{
		5 * WHITE_KEY_W - BLACK_KEY_W/2, 0, BLACK_KEY_W, BLACK_KEY_H, "G#", 415.30, 0, 8
	},
	{
		5 * WHITE_KEY_W, 0, WHITE_KEY_W, WHITE_KEY_H , "A", 440.00, 1, 9
	},
	{
		6 * WHITE_KEY_W - BLACK_KEY_W/2, 0, BLACK_KEY_W, BLACK_KEY_H, "A#", 466.16, 0, 10
	},
	{
		6 * WHITE_KEY_W, 0, WHITE_KEY_W, WHITE_KEY_H , "H", 493.88, 1, 11
	},
};

const uint32_t whiteKeysIndices[] = { 0, 2, 4, 5, 7, 9, 11 };
const uint32_t blackKeysIndices[] = { 1, 3, 6, 8, 10 };

void DrawPiano(void);
void DisplayInit(void);
void HandleTouchInput();
uint32_t DetectKeyPress(uint16_t x, uint16_t y, int index);

TS_Init_t _hTS;

void DisplayInit(void)
{
	uint32_t x_size, y_size;

	BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);

	BSP_LCD_GetXSize(0, &x_size);
	BSP_LCD_GetYSize(0, &y_size);

	_hTS.Width = x_size;
	_hTS.Height = y_size;
	_hTS.Orientation = TS_SWAP_XY;
	_hTS.Accuracy = 5;

    UTIL_LCD_SetFuncDriver(&LCD_Driver);
    UTIL_LCD_SetLayer(0);
    UTIL_LCD_SetFont(&Font16);

    if (BSP_TS_Init(0, &_hTS) != BSP_ERROR_NONE) {
    	UTIL_LCD_DisplayStringAt(0, 220, (uint8_t*)"TS_Init Failed!", CENTER_MODE);
    }
}

void DrawPiano(void)
{
	for (int i = 0; i < 7; i++)
	{
		int idx = whiteKeysIndices[i];
		DrawWhiteKey(&keys[idx]);
	}

	for (int i = 0; i < 5; i++)
	{
		int idx = blackKeysIndices[i];
		DrawBlackKey(&keys[idx]);
	}

}

void DrawKey(Key* key, uint32_t offset, uint32_t drawColor, uint32_t fillColor, uint32_t textColor, uint32_t backColor)
{
	UTIL_LCD_FillRect(key->x, key->y, key->width, key->height, fillColor);
	UTIL_LCD_DrawRect(key->x, key->y, key->width, key->height, drawColor);
	UTIL_LCD_SetTextColor(textColor);
	UTIL_LCD_SetBackColor(backColor);
	UTIL_LCD_DisplayStringAt(key->x + key->width / 2 - offset, key->height - 30, (uint8_t*)key->symbol, LEFT_MODE);
}

void DrawBlackKey(Key* key)
{
	DrawKey(key,
			10,
			UTIL_LCD_COLOR_BLACK,
			UTIL_LCD_COLOR_BLACK,
			UTIL_LCD_COLOR_WHITE,
			UTIL_LCD_COLOR_BLACK);
}

void DrawWhiteKey(Key* key)
{
	DrawKey(key,
			5,
			UTIL_LCD_COLOR_BLACK,
			UTIL_LCD_COLOR_WHITE,
			UTIL_LCD_COLOR_BLACK,
			UTIL_LCD_COLOR_WHITE);
}

void DrawGrayKey(Key* key)
{
	DrawKey(key,
			key->white ? 5 : 10,
			UTIL_LCD_COLOR_GRAY,
			UTIL_LCD_COLOR_GRAY,
			UTIL_LCD_COLOR_WHITE,
			UTIL_LCD_COLOR_GRAY);
}

void PressKey(uint32_t index)
{
	DrawGrayKey(&keys[index]);
}

void ReleaseKey(uint32_t index)
{
	uint32_t keyCount = sizeof(keys) / sizeof(keys[0]);

	if (index >= keyCount)
		return;

	keys[index].pressed = 0;

	if (!keys[index].white)
	{
		DrawBlackKey(&keys[index]);
		return;
	}

	for (int i = (index == 0 ? 0 : index - 1); i <= index + 1 && i < keyCount; i++)
	{
		if (keys[i].white)
		{
			DrawWhiteKey(&keys[i]);
		}
	}

		for (int i = (index == 0 ? 0 : index - 1); i <= index + 1 && i < keyCount; i++)
	{
		if (!keys[i].white)
		{
			DrawBlackKey(&keys[i]);
		}
	}

	if (strcmp(keys[index].symbol, "F") == 0)
	{
		DrawBlackKey(&keys[3]);
	}
	else if (strcmp(keys[index].symbol, "E") == 0)
	{
		DrawBlackKey(&keys[6]);
	}
}


void HandleTouchInput()
{
    TS_MultiTouch_State_t ts;
    BSP_TS_Get_MultiTouchState(0, &ts);

    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
    {
        keys[i].currentlyTouched = 0;
    }

    for (int t = 0; t < ts.TouchDetected; t++)
    {
        uint16_t x = ts.TouchX[t];
        uint16_t y = ts.TouchY[t];

        // First check black keys
        for (int j = 0; j < sizeof(blackKeysIndices)/sizeof(blackKeysIndices[0]); j++)
        {
            int idx = blackKeysIndices[j];
            if (DetectKeyPress(x, y, idx))
            {
                keys[idx].currentlyTouched = 1;
            }
        }

        // Then check white keys
        for (int j = 0; j < sizeof(whiteKeysIndices)/sizeof(whiteKeysIndices[0]); j++)
        {
            int idx = whiteKeysIndices[j];
            if (DetectKeyPress(x, y, idx))
            {
                keys[idx].currentlyTouched = 1;
            }
        }
    }

    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
    {
        if (keys[i].currentlyTouched && !keys[i].pressed)
        {
            PressKey(i);
            keys[i].pressed = 1;
        } else if (!keys[i].currentlyTouched && keys[i].pressed)
        {
            ReleaseKey(i);
            keys[i].pressed = 0;
        }
    }
}

uint32_t DetectKeyPress(uint16_t x, uint16_t y, int index)
{
	if (x >= keys[index].x &&
		x <= (keys[index].x + keys[index].width) &&
		y >= keys[index].y &&
		y <= (keys[index].y + keys[index].height))
	{
		return 1;
	}
	return 0;
}

Key* GetPressedKeys(uint32_t* outCount)
{
	TS_MultiTouch_State_t ts;
	BSP_TS_Get_MultiTouchState(0, &ts);

	if (!ts.TouchDetected)
	{
		return NULL;
	}

	uint32_t noOfKeysPressed = 0;
	for (uint32_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
	{
		if (keys[i].pressed)
		{
			noOfKeysPressed++;
		}
	}

	if (noOfKeysPressed == 0)
	{
		*outCount = 0;
		return NULL;
	}

	Key* keysPressed = (Key*) malloc(noOfKeysPressed * sizeof(Key));
	if (!keysPressed)
	{
		*outCount = 0;
		return NULL;
	}

	uint32_t index = 0;

	for (uint32_t i = 0; i <  sizeof(keys) / sizeof(keys[0]); i++)
	{
		if (keys[i].pressed)
		{
			keysPressed[index++] = keys[i];
		}
	}

	*outCount = noOfKeysPressed;
	return keysPressed;
}
