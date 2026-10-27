/* framer.c - see framer.h for the ownership rules. */
#include "framer.h"
#include <string.h>

#ifdef FRAMER_HOST_TEST
/* PC build: no interrupts, so the lock does nothing */
static inline uint32_t lock_irq(void)            { return 0u; }
static inline void     unlock_irq(uint32_t prev) { (void)prev; }
#else
#include "stm32f0xx.h"
static inline uint32_t lock_irq(void)
{
    uint32_t prev = __get_PRIMASK();   /* remember whether IRQs were already off */
    __disable_irq();
    return prev;
}
static inline void unlock_irq(uint32_t prev) { __set_PRIMASK(prev); } /* restore, don't blindly enable */
#endif

#define BARRIER() __asm volatile("" ::: "memory")   /* stop the compiler reordering */

static Frame            q[FRAMER_QSLOTS];
static volatile uint8_t q_head;        /* next slot the ISR fills  (ISR writes)  */
static volatile uint8_t q_tail;        /* next slot main reads     (main writes) */
static volatile uint32_t dropped;

static char              line[FRAMER_MAX_LEN + 1];   /* the "notepad" (ISR owns it) */
static volatile uint8_t  line_len;
static volatile bool     discarding;
static volatile uint32_t last_byte_ms;

void framer_init(void)
{
    q_head = q_tail = 0;
    dropped = 0;
    line_len = 0;
    discarding = false;
    last_byte_ms = 0;
}

/* ISR only. Never overwrites a pending frame. */
static void push(const char *text, uint8_t len, uint8_t err)
{
    uint8_t next = (uint8_t)((q_head + 1u) % FRAMER_QSLOTS);
    if (next == q_tail) {              /* full */
        dropped++;
        return;
    }
    if (len > FRAMER_MAX_LEN) len = FRAMER_MAX_LEN;
    memcpy(q[q_head].text, text, len);
    q[q_head].text[len] = '\0';
    q[q_head].len = len;
    q[q_head].err = err;
    BARRIER();                         /* finish writing the slot ... */
    q_head = next;                     /* ... THEN publish it */
}

void framer_feed(uint8_t c, uint32_t now_ms)
{
    last_byte_ms = now_ms;

    if (c == '\r') return;                         /* LF and CRLF both work */

    if (c == '\n') {
        if (discarding) {
            discarding = false;                    /* end of an oversized line (its error is already queued) */
        } else {
            push(line, line_len, FRAME_OK);        /* a newline is checked BEFORE the length, so 31 chars + \n is fine */
        }
        line_len = 0;
        return;
    }

    if (discarding) return;                        /* throw away until newline */

    if (line_len < FRAMER_MAX_LEN) {
        line[line_len++] = (char)c;
    } else {                                       /* a 32nd real character: too long */
        push("", 0, FRAME_TOO_LONG);
        discarding = true;
        line_len = 0;
    }
}

void framer_stream_error(void)
{
    /* ISR context. Whatever was half-received can't be trusted. */
    line_len = 0;
    discarding = true;                             /* ignore until newline (or timeout) */
}

void framer_check_timeout(uint32_t now_ms)
{
    uint32_t prev = lock_irq();                    /* the ISR can't touch the notepad in here */
    if ((line_len > 0u || discarding) &&
        (int32_t)(now_ms - last_byte_ms) >= (int32_t)FRAMER_TIMEOUT_MS) {
        line_len = 0;
        discarding = false;
    }
    unlock_irq(prev);
}

bool framer_get(Frame *out)
{
    if (q_tail == q_head) return false;            /* empty */
    *out = q[q_tail];                              /* private copy */
    BARRIER();
    q_tail = (uint8_t)((q_tail + 1u) % FRAMER_QSLOTS);   /* now the ISR may reuse the slot */
    return true;
}

uint32_t framer_dropped(void) { return dropped; }
