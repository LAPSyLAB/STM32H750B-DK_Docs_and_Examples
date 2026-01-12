/*
 * bsp_audio.c
 *
 *  Created on: Sep 15, 2025
 *      Author: nejcs
 */

#include "bsp_audio.h"

#define AUDIO_SAMPLE_RATE AUDIO_FREQUENCY_96K
#define AUDIO_BITS 		  AUDIO_RESOLUTION_16B
#define AUDIO_CHANNELS	  2
#define AUDIO_BUFFER_SIZE 2048
#define PI 				  3.14159265f

int16_t audioBuffer[AUDIO_BUFFER_SIZE * AUDIO_CHANNELS];
static volatile uint8_t bufferState = 0;
static float keyPhases[12] = { 0.0f };
static float phase = 0.0f;
float phases[12];
float freqs[12];
uint32_t indices[12];


void AudioInit(void);
void StartAudio(void);
void DMA2_Stream1_IRQHandler(void);

void BSP_AUDIO_Init(void);
void BSP_AUDIO_OUT_TransferComplete_CallBack(uint32_t Instance);
void BSP_AUDIO_OUT_HalfTransfer_CallBack(uint32_t Instance);
void BSP_AUDIO_OUT_Error_CallBack(uint32_t Instance);

void DMA2_Stream1_IRQHandler(void)
{
   BSP_AUDIO_OUT_IRQHandler(0);
}

void AudioInit(void)
{
	RCC_PeriphCLKInitTypeDef periphClkInitStruct = {0};
	periphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
	periphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL3;
	periphClkInitStruct.PLL3.PLL3M = 16;
	periphClkInitStruct.PLL3.PLL3N = 344;
	periphClkInitStruct.PLL3.PLL3P = 7;
	periphClkInitStruct.PLL3.PLL3Q = 7;
	periphClkInitStruct.PLL3.PLL3R = 7;
	HAL_RCCEx_PeriphCLKConfig(&periphClkInitStruct);

	BSP_AUDIO_Init();
}

void StartAudio()
{
    uint32_t pressedCount;
    Key* pressedKeys = GetPressedKeys(&pressedCount);

    if (pressedKeys == NULL || pressedCount == 0)
    {
    	return;
    }

    if (pressedCount > 12)
    {
    	pressedCount = 12;
    }

    if (bufferState == 1)
    {
        ProcessBuffer(&audioBuffer[0], AUDIO_BUFFER_SIZE / 2, pressedKeys, pressedCount);
        bufferState = 0;
    }
    else if (bufferState == 2)
    {
    	ProcessBuffer(&audioBuffer[AUDIO_BUFFER_SIZE / 2 * AUDIO_CHANNELS], AUDIO_BUFFER_SIZE / 2, pressedKeys, pressedCount);
        bufferState = 0;
    }

    free(pressedKeys);
}

void ProcessBuffer(int16_t* buf, uint32_t length, Key* pressedKeys, uint32_t pressedCount)
{
    memset(buf, 0, length * AUDIO_CHANNELS * sizeof(int16_t));
    for (uint32_t i = 0; i < pressedCount; i++)
    {
    	if (pressedKeys[i].index > 11)
    	{
    		continue;
    	}

    	MixKeyIntoBuffer(audioBuffer, length, pressedKeys[i].frequency, pressedKeys[i].index);
    }


    SCB_CleanDCache_by_Addr((uint32_t*) buf, length * AUDIO_CHANNELS * sizeof(int16_t));
}


void MixKeyIntoBuffer(int16_t* buf, uint32_t length, float freq, uint32_t keyIndex)
{
	float step = 2.0f * PI * freq / AUDIO_SAMPLE_RATE;

	for (uint32_t i = 0; i < length; i++)
	{
		int16_t sample = (int16_t)(32767.0f * 0.2f * sinf(keyPhases[keyIndex])); // 0.2 to avoid clipping

		buf[2*i]   += sample;
		buf[2*i+1] += sample;

		keyPhases[keyIndex] += step;
		if (keyPhases[keyIndex] >= 2.0f * PI)
		{
			keyPhases[keyIndex] -= 2.0f * PI;
		}
	}
}

void FillAudioBuffer(int16_t* buf, uint32_t length, float freq)
{
	float step = 2.0f * PI * freq / AUDIO_SAMPLE_RATE;
    for (uint32_t i = 0; i < length; i++)
    {
    	int16_t sample = (int16_t)(32767.0f * sinf(phase));

    	buf[2*i] = sample;  // Left
    	buf[2*i + 1] = sample;  // Right

    	phase += step;
    	if (phase > 2.0f * PI)
    	{
    		phase -= 2.0f * PI;
    	}
    }
}

void BSP_AUDIO_Init(void)
{
	BSP_AUDIO_Init_t audioInit;
    audioInit.Device = AUDIO_OUT_DEVICE_HEADPHONE;
    audioInit.SampleRate = AUDIO_SAMPLE_RATE;
    audioInit.BitsPerSample = AUDIO_BITS;
    audioInit.ChannelsNbr = AUDIO_CHANNELS;
    audioInit.Volume = 100;

    if (BSP_AUDIO_OUT_Init(0, &audioInit) != BSP_ERROR_NONE)
    {
    	Error_Handler();
    }

    BSP_AUDIO_OUT_Play(0, (uint8_t*) audioBuffer, AUDIO_BUFFER_SIZE * AUDIO_CHANNELS * sizeof(int16_t));
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(uint32_t Instance)
{
    bufferState = 1;
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(uint32_t Instance)
{
    bufferState = 2;
}

void BSP_AUDIO_OUT_Error_CallBack(uint32_t Instance)
{
	Error_Handler();
}
