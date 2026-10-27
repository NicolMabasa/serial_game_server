/* server.c - UART glue.
 *
 * RECEIVE : direct-register interrupt (fast: the HAL receive path was too slow
 *           at 8 MHz and dropped bytes of back-to-back commands).
 * TRANSMIT: HAL_UART_Transmit from the main loop, with a timeout.
 * All real work happens in server_poll() on the main loop. */
#include "server.h"
#include "framer.h"
#include "protocol.h"
#include "game.h"

#define TX_TIMEOUT_MS 100u

static UART_HandleTypeDef *uart;

void server_init(UART_HandleTypeDef *huart)
{
    uart = huart;
    game_init();
    framer_init();

    /* clear any stale error flags, then turn the receive interrupt on */
    USART1->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NCF;
    USART1->CR1 |= USART_CR1_RXNEIE;
    USART1->CR3 |= USART_CR3_EIE;
}

/* --- interrupt context -------------------------------------------------- */

int server_uart_isr(void)
{
    uint32_t isr = USART1->ISR;

    if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE)) {
        USART1->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NCF;
        framer_stream_error();            /* stream is garbled: drop partial frame, skip to newline */
    }
    if (isr & USART_ISR_RXNE) {
        framer_feed((uint8_t)USART1->RDR, HAL_GetTick());   /* reading RDR clears RXNE */
    }
    return 1;
}

/* --- main loop ---------------------------------------------------------- */

void server_poll(void)
{
    framer_check_timeout(HAL_GetTick());

    Frame f;
    if (framer_get(&f)) {
        char reply[PROTOCOL_REPLY_MAX + 2];
        size_t n = protocol_handle(f.text, f.len, f.err, reply);
        reply[n++] = '\n';
        (void)HAL_UART_Transmit(uart, (uint8_t *)reply, (uint16_t)n, TX_TIMEOUT_MS);
    }
}
