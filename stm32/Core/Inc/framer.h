/* framer.h - byte stream -> newline-terminated frames -> 3-slot FIFO queue.
 * No HAL: the ISR calls framer_feed(), the main loop calls the rest.
 *
 * Ownership (this is what makes it safe without locking the queue):
 *   - the ISR owns the line buffer, 'discarding', and the queue HEAD index
 *   - the main loop owns the queue TAIL index
 *   - the only shared "write" is the timeout reset, done inside a short
 *     interrupt-off section that saves and restores PRIMASK.
 */
#ifndef FRAMER_H
#define FRAMER_H

#include <stdint.h>
#include <stdbool.h>

#define FRAMER_MAX_LEN     31u   /* characters in a command (newline not stored) */
#define FRAMER_QSLOTS      4u    /* array size; one slot kept empty => 3 usable */
#define FRAMER_TIMEOUT_MS  500u

#define FRAME_OK        0u
#define FRAME_TOO_LONG  1u

typedef struct {
    char    text[FRAMER_MAX_LEN + 1];
    uint8_t len;
    uint8_t err;      /* FRAME_OK or FRAME_TOO_LONG */
} Frame;

void     framer_init(void);

/* ISR context */
void     framer_feed(uint8_t c, uint32_t now_ms);
void     framer_stream_error(void);        /* UART hardware error: stream is garbled */

/* main-loop context */
void     framer_check_timeout(uint32_t now_ms);
bool     framer_get(Frame *out);           /* copies oldest frame out, frees its slot */
uint32_t framer_dropped(void);             /* frames lost because the queue was full */

#endif
