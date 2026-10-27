/* protocol.h - turns one received frame into one reply string. Plain C, no HAL. */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define PROTOCOL_REPLY_MAX 24   /* longest reply "OK:BOARD:xxxxxxxxx" + NUL fits */

/* text/len : the frame's characters (no newline). err != 0 : framer already
 * flagged the frame as bad (e.g. too long).
 * out      : buffer of at least PROTOCOL_REPLY_MAX bytes, receives the reply
 *            WITHOUT a newline. Returns the reply length. */
size_t protocol_handle(const char *text, uint8_t len, uint8_t err, char *out);

#endif
