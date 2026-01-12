#ifndef ADC_H_
#define ADC_H_

#include <stm32h7xx.h>
#include <stdint.h>
#include <stdio.h>
#include "systick.h"

void pf8_adc_init(void);
void adc_calibrate(void);
void start_single_conversion(void);
void start_continuous_conversion(void);
uint32_t adc_read(void);

#endif /* ADC_H_ */
