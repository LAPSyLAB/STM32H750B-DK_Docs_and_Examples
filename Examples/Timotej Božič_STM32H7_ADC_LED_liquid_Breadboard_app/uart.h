#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <stdio.h>
#include "stm32h750xx.h"

void uart3_tx_init(void);
void uart3_rx_init(void);

#endif /* UART_H_ */
