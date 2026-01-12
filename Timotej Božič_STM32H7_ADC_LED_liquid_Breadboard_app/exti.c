#include "exti.h"

// Water sensor: 		D7 -> PI8

#define GPIOIEN			(1u << 8)

#define	SYSCFGEN		(1u << 1)

#define WATER_SENS_PIN	PIN8

void pi8_exti_init(void)
{
	// Disable global interrupts
	__disable_irq();

	// Connect clock to PI
	RCC->AHB4ENR |= GPIOIEN;

	// Set PI8 as input pin (00)
	GPIOI->MODER &= ~(1u << 16);
	GPIOI->MODER &= ~(1u << 17);

	// Connect clock to SYSCFG
	RCC->APB4ENR |= SYSCFGEN;

	// Select PORTI for EXTI8
	SYSCFG->EXTICR[2] |= (1u << 3);

	// Unmask EXTI8
	EXTI->IMR1 |= (1u << 8);

	// Select rising edge trigger
	EXTI->RTSR1 |= (1u << 8);

	// Enable EXTI line in NVIC
	NVIC_EnableIRQ(EXTI9_5_IRQn);

	// Enable global interrupts
	__enable_irq();
}
