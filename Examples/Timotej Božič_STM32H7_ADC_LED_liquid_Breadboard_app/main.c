#include "stm32h750xx.h"
#include <stdio.h>
#include "uart.h"
#include "systick.h"
#include "adc.h"
#include "fpu.h"
#include "exti.h"

// Water sensor: 		D7 -> PI8
// Light sensor:		A1 -> PF8
// External water LED:	D8 -> PE3
// External light LED:	D4 -> PK1


#define GPIOEEN			(1u << 4)
#define GPIOIEN			(1u << 8)
#define GPIOKEN			(1u << 10)

#define PIN3			(1u << 3)
#define PIN8			(1u << 8)
#define	PIN1			(1u << 1)

#define EXT_WATER_LED	PIN3
#define	EXT_LIGHT_LED	PIN1
#define LIGHT_SENS_PIN	PIN8
#define WATER_SENS_PIN	PIN8


int main(void)
{
	fpu_enable();

	// connect clock to PE, PK
	RCC->AHB4ENR |= (GPIOEEN | GPIOKEN);

	// set PE3 (external water LED) to output mode (01)
	GPIOE->MODER |=  (1u << 6);
	GPIOE->MODER &= ~(1u << 7);

	// set PK1 (external light LED) to output mode (01)
	GPIOK->MODER |=  (1u << 2);
	GPIOK->MODER &= ~(1u << 3);

	uart3_tx_init();
	systick_init();
	pf8_adc_init();
	pi8_exti_init();

	// Start temperature conversion
	start_continuous_conversion();

	uint32_t sensor_val;
	double light_percentage;

	while(1)
	{

		// start_single_conversion();
		sensor_val = adc_read();

		light_percentage = (sensor_val / 65536.0) * 100;

		printf("Light level: %d (%2.2f%%)\n\r", (int) sensor_val, light_percentage);

		if( sensor_val < 2500 ) {
			// very little light has been detected
			// turn on the lights
			GPIOK->BSRR = (1u << 1);
		}
		else {
			// a decent amount of light has been detected
			// turn off the lights
			GPIOK->BSRR = (1u << 17);

		}

		systick_delay_ms(500);
	}

	systick_terminate();
}

static void exti_water_detected(void)
{
	// while water is deteced
	printf("WATER DETECTED\n\r");

	// turn off the lights
	GPIOK->BSRR = (1u << 17);
	while( (GPIOI->IDR & WATER_SENS_PIN) )
	{
		// enable external LED (on pin 3)
		GPIOE->BSRR = (1u << 3);
		systick_delay_ms(500);
		// disable external LED
		GPIOE->BSRR = (1u << 19);
		systick_delay_ms(500);
	}

	printf("WATER CLEARED\n\r");
}

void EXTI9_5_IRQHandler(void)
{
	if( (EXTI->PR1 & LINE8) )
	{
		// Clear PR
		EXTI->PR1 |= LINE8;

		// Do something
		exti_water_detected();
	}
}
