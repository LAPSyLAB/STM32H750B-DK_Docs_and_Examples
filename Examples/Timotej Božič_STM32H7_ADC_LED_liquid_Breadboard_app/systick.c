#include "stm32h7xx.h"
#include <stdio.h>

#define SYS_FREQ			((unsigned int) 480e6)
#define SYSTICK_LOAD_VAL	(SYS_FREQ / ((unsigned int) 10e3))

#define CTRL_ENABLE			(1u << 0)
#define CTRL_CLKSRC			(1u << 2)
#define CTRL_COUNTFLG		(1u << 16)

void systick_init(void)
{
	// Reload with number of clocks per ms
	SysTick->LOAD = SYSTICK_LOAD_VAL;

	// clear systick current value register
	SysTick->VAL = 0;

	// enable systick and select internal clock source
	SysTick->CTRL = CTRL_ENABLE | CTRL_CLKSRC;
}

void systick_terminate(void)
{
	SysTick->CTRL = 0;
}

void systick_delay_ms(int delay)
{
	for(int i = 0; i < delay; ++i)
	{
		// wait until count flag is set
		while(!(SysTick->CTRL & CTRL_COUNTFLG)) {}
	}
}
