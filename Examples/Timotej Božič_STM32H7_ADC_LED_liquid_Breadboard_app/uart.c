#include "uart.h"

#define GPIOJEN				(1u << 9) // 0010 0000 0000

#define GPIOBEN		(1u << 1)
#define UART3EN		(1u << 18)

#define CR1_TE		(1u << 3)
#define CR1_RE		(1u << 2)
#define CR1_UE		(1u << 0)
#define SR_TXE		(1u << 7)
#define SR_RXE		(1u << 5)

#define SYS_FREQ	((unsigned int) 64e6)
#define APB1_CLK	SYS_FREQ

#define UART_BAUDRATE	115200

void uart3_tx_init(void);
void uart3_rx_init(void);
static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t periph_clock, uint32_t baudrate);
static int32_t compute_uart_baudrate(uint32_t periph_clock, uint32_t baudrate);
void uart3_write(int ch);
int  uart3_read(void);

int __io_putchar(int ch)
{
	uart3_write(ch);
	return ch;
}

void uart3_tx_init(void)
{
	/* configure uart gpio pin */
	// enable clock access to gpiob
	RCC->AHB4ENR |= GPIOBEN;

	// set PB10 to alternate function mode
	GPIOB->MODER &= ~(1u << 20);
	GPIOB->MODER |= (1u << 21);

	// set PB10 to alternate function type to UART_TX (AF7)
	GPIOB->AFR[1] |= (1u << 8);
	GPIOB->AFR[1] |= (1u << 9);
	GPIOB->AFR[1] |= (1u << 10);
	GPIOB->AFR[1] &= ~(1u << 11);


	/* Configure the UART module */
	// enable clock access to uart3
	RCC->APB1LENR |= UART3EN;

	// when configuring usart it should be disabled
	USART3->CR1 &= ~(CR1_UE);

	// configure baudrate
	uart_set_baudrate(USART3, APB1_CLK, UART_BAUDRATE);

	// configure transfer direction
	USART3->CR1 |= CR1_TE;

	// enable uart module
	USART3->CR1 |= CR1_UE;
}

void uart3_rx_init(void)
{
	/* configure uart gpio pin */
	// enable clock access to gpiob
	RCC->AHB4ENR |= GPIOBEN;

	// set PB11 to alternate function mode
	GPIOB->MODER &= ~(1u << 22);
	GPIOB->MODER |= (1u << 23);

	// set PB11 to alternate function type to UART_TX (AF7)
	GPIOB->AFR[1] |= (1u << 12);
	GPIOB->AFR[1] |= (1u << 13);
	GPIOB->AFR[1] |= (1u << 14);
	GPIOB->AFR[1] &= ~(1u << 15);


	/* Configure the UART module */
	// enable clock access to uart3
	RCC->APB1LENR |= UART3EN;

	// when configuring usart it should be disabled
	USART3->CR1 &= ~(CR1_UE);

	// configure baudrate
	uart_set_baudrate(USART3, APB1_CLK, UART_BAUDRATE);

	// configure transfer direction
	USART3->CR1 |= CR1_RE;

	// enable uart module
	USART3->CR1 |= CR1_UE;
}

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t periph_clock, uint32_t baudrate)
{
	USARTx->BRR = compute_uart_baudrate(periph_clock, baudrate);
}

static int32_t compute_uart_baudrate(uint32_t periph_clock, uint32_t baudrate)
{
	return (periph_clock/baudrate);
}

void uart3_write(int ch)
{
	// make sure transmit data register is empty
	while( !(USART3->ISR & SR_TXE) ) {}

	// write to transmit data register
	USART3->TDR = (ch & 0xFF);
}

int uart3_read(void)
{
	// make sure data is ready to be read
	while( !(USART3->ISR & SR_RXE) ) {}

	// read the data from the register
	return (USART3->RDR & 0xFF);
}

