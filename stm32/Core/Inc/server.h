/* server.h - glue between the UART hardware, the framer, the protocol and the game. */
#ifndef SERVER_H
#define SERVER_H

#include "stm32f0xx_hal.h"

void server_init(UART_HandleTypeDef *huart);   /* call once, after MX_USART1_UART_Init() */
void server_poll(void);                         /* call every pass of the main loop */

/* Called first thing from USART1_IRQHandler (stm32f0xx_it.c). Always returns 1,
 * meaning "handled, do not call the (slow) HAL handler". */
int  server_uart_isr(void);

#endif
