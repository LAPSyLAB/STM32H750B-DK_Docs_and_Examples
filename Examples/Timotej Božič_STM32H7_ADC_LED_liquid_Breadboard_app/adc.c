// Temperature sensor:	A1 -> PF8

#include "adc.h"

#define ADC3EN		(1u << 24)
#define ADC_CH7 	(7u << 6)


#define CR_ADEN		(1u << 0)
#define CR_ADDIS	(1u << 1)

#define CR_ADSTART	(1u << 2)
#define	CR_ADSTP	(1u << 4)

#define	CR_ADVREGEN (1u << 28)
#define CR_DEEPPWD	(1u << 29)

#define CR_ADCALLIN (1u << 16)
#define CR_ADCALDIF (1u << 30)
#define CR_ADCAL	(1u << 31)


#define CALFACT_SINGLE 		(1u << 0)
#define CALFACT_SINGLE_MASK (0x7FF) // (0111 1111 1111)
#define CALFACT_LENGTH	 	(11u)


#define ISR_EOC		(1u << 2)
#define ISR_EOS		(1u << 3)

#define ISR_ADRDY	(1u << 0)
#define ISR_LDORDY	(1u << 12)


#define	PCSEL_CH3	(1u << 3)
#define	PCSEL_CH7	(1u << 7)


#define CFGR_CONT	(1u << 13)
#define CFGR_RES	(6u << 2)


#define SMPR1_SMP7	(7u << 21)


#define GPIOFEN		(1u << 5)

#define PIN8		(1u << 8)
#define TEMP_PIN	PIN8

void pf8_adc_init(void)
{

	/***** Setup *****/
	// Enable clock access to GPIOF
	RCC->AHB4ENR |= GPIOFEN;

	// Set PF8 to analog mode (11)
	GPIOF->MODER |= (1u << 16);
	GPIOF->MODER |= (1u << 17);

	// Enable clock access to ADC
	RCC->AHB4ENR |= ADC3EN;

	// Configure the ADC independent clock
	ADC3_COMMON->CCR |=  (1u << 17);
	ADC3_COMMON->CCR &= ~(1u << 16);

	/***** Enabling the ADC *****/
	// Exit ADC deep-power-down mode (DEEPPWD=0)
	ADC3->CR &= ~(CR_DEEPPWD);

	// Enable the internal voltage regulator (ADVREGEN=1)
	ADC3->CR |= CR_ADVREGEN;

	// Wait for the startup time of the regulator
	while( !(ADC3->ISR & ISR_LDORDY) ) {}

	// Calibrate the ADC
	// adc_calibrate();

	// Configure the ADC (ADC_CCR, ADC_DIFSEL_DIFSEL, ADC_CR_ADEN, ADC_CR_ADCAL)
	// Clear ADRDY flag
	ADC3->ISR |= ISR_ADRDY;

	// Enable the ADC (ADEN=1)
	ADC3->CR |= CR_ADEN;

	// Wait for ADC to be ready for operation (ADRDY=1)
	while( !(ADC3->ISR & ISR_ADRDY) ) {}

	// Clear the ADRDY flag
	ADC3->ISR |= ISR_ADRDY;

	// Preselect the channels and their corresponding channels (ADC3_INP3 + ADC3_INP7)
	ADC3->PCSEL |= PCSEL_CH3;
	ADC3->PCSEL |= PCSEL_CH7;

	// Configure the ADC (ADC_CFGR, ADC_SMPRy, ADC_TRy, ADC_SQRy, ADC_JDRy, ADC_OFRy and ADC_IER)
	// Select the channel
	ADC3->SQR1 = ADC_CH7;

	// Set the length (length 1 = 0000)
	ADC3->SQR1 &= ~(1u << 0);
	ADC3->SQR1 &= ~(1u << 1);
	ADC3->SQR1 &= ~(1u << 2);
	ADC3->SQR1 &= ~(1u << 3);

	// Set the Sampling time to 810 cycles
	//ADC3->SMPR1 |= SMPR1_SMP7;

	// Use 10-bit resolution
	//ADC3->CFGR |= CFGR_RES;
}

void pf8_adc_deinit(void)
{
	/***** Disabling the ADC *****/
	// Check that ADC is not converting (ADSTART=0 and JADSTART=0)
	// If any conversion is going on stop it (ADSTP=1 and JADSTP=1)
	// If stopping conversion wait for (ADSTP=0 and JADSTP=0)
	// Disable the ADC (ADDIS=1)
	// Wait until the ADC has turned off (ADEN=0)
	// Disable the voltage regulator (ADVREGEN=0)
	// Re-enter the deep-power-down mode (DEEPPWD=1)

}

void adc_calibrate(void)
{
	char wasEnabled = 0;
	/***** Calibration *****/
	// Set type of calibration (single-ended->ADCALDIF=0, differential->ADCALDIF=1)
	ADC3->CR &= ~(CR_ADCALDIF);

	// Also do linear calibration (ADCALLIN=1)
	ADC3->CR |= CR_ADCALLIN;

	// Disable the ADC (ADEN=0)
	if( (ADC3->CR & CR_ADEN) )
	{
		wasEnabled = 1;

		// Disable the ADC
		ADC3->CR |= CR_ADDIS;

		// Wait for ADC to shutdown
		while( !(ADC3->CR & CR_ADDIS) ) {}
	}
	// Start calibration (ADCAL=1)
	ADC3->CR |= CR_ADCAL;

	// Wait for calibration to finish (until ADCAL=0)
	while( !(ADC3->CR & CR_ADCAL) ) {}

	// Enable the ADC if it was enabled before calibrating (ADEN=1)
	if( wasEnabled )
	{
		// Clear ADRDY flag
		ADC3->ISR |= ISR_ADRDY;

		// Enable the ADC (ADEN=1)
		ADC3->CR |= CR_ADEN;

		// Wait for ADC to be ready for operation (ADRDY=1)
		while( !(ADC3->ISR & ISR_ADRDY) ) {}
	}

	// Read out the calibration factor (ADC_CALFACT)
	unsigned int cal_factor = (ADC3->CALFACT & CALFACT_SINGLE_MASK);
	printf("Calibration factor: %d\n\r", cal_factor);
}

void start_single_conversion(void)
{
	// Configure ADC for Single conversion mode (CONT=0)
	ADC3->CFGR &= ~(CFGR_CONT);

	// Start the ADC conversion
	ADC3->CR |= CR_ADSTART;
}

void start_continuous_conversion(void)
{
		// Configure ADC for Single conversion mode (CONT=0)
		ADC3->CFGR |= CFGR_CONT;

		// Start the ADC conversion
		ADC3->CR |= CR_ADSTART;
}

uint32_t adc_read(void)
{
	// wait for conversion to be complete
	// !(ADC3->ISR & ISR_EOS) ||
	while( !(ADC3->ISR & ISR_EOS) && !(ADC3->ISR & ISR_EOC) ) {}

	// Clear EOS flag
	ADC3->ISR |= ISR_EOS;

	// read converted result
	uint32_t data = (ADC3->DR);

	// VREF+ = 3.3 V x VREFINT_CAL / VREFINT_DATA
	// int32_t v_ref = 3.3 V * VREFINT_CAL / VREFINT_DATA;

	// (ReadData + 32768) * Vref/(gain * 65535)

	return data;
}
