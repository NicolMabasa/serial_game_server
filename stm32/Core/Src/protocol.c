/* protocol.c - command parsing and replies. Plain C, no HAL.
 *
 * Order of checks for every frame:
 *   1. framer error / empty / non-printable  -> NACK:BADCMD
 *   2. unknown command or bad MOVE syntax    -> NACK:BADCMD
 *   3. command not allowed in current state  -> NACK:NOGAME (or BADCMD for NEW)
 *   4. range, then occupied                  -> NACK:RANGE / NACK:OCCUPIED
 * Nothing is changed until every check has passed.
 */
#include "protocol.h"
#include "game.h"
#include <string.h>

static size_t put(char *out, const char *s)
{
    size_t n = strlen(s);
    memcpy(out, s, n + 1);
    return n;
}

static size_t put_board(char *out, const char *prefix)
{
    size_t n = put(out, prefix);
    n += put(out + n, game_board());
    return n;
}

static int is(const char *t, uint8_t len, const char *word)
{
    size_t n = strlen(word);
    return (len == n) && (memcmp(t, word, n) == 0);
}

size_t protocol_handle(const char *t, uint8_t len, uint8_t err, char *out)
{
    if (err != 0 || len == 0) return put(out, "NACK:BADCMD");

    for (uint8_t i = 0; i < len; i++) {            /* printable ASCII only */
        if ((unsigned char)t[i] < 0x20 || (unsigned char)t[i] > 0x7E)
            return put(out, "NACK:BADCMD");
    }

    if (is(t, len, "CONNECT")) {
        game_init();                               /* new session starts IDLE */
        return put(out, "OK");
    }
    if (is(t, len, "DISCONNECT")) {
        game_init();
        return put(out, "OK");
    }
    if (is(t, len, "NEW")) {
        if (game_state() == GS_IN_PROGRESS) return put(out, "NACK:BADCMD");
        game_new();
        return put_board(out, "OK:BOARD:");
    }
    if (is(t, len, "STATE")) {
        if (game_state() == GS_IDLE) return put(out, "NACK:NOGAME");
        return put_board(out, "BOARD:");
    }
    if (is(t, len, "RESIGN")) {
        if (game_state() != GS_IN_PROGRESS) return put(out, "NACK:NOGAME");
        game_resign();
        return put(out, "OK:GAMEOVER");
    }

    if (len >= 5 && memcmp(t, "MOVE:", 5) == 0) {
        uint8_t i = 5;
        int neg = 0;
        long v = 0;
        if (i < len && t[i] == '-') { neg = 1; i++; }
        if (i >= len) return put(out, "NACK:BADCMD");      /* "MOVE:" or "MOVE:-" */
        for (; i < len; i++) {
            if (t[i] < '0' || t[i] > '9') return put(out, "NACK:BADCMD");
            if (v < 1000) v = v * 10 + (t[i] - '0');         /* cap: stays out of range */
        }
        if (neg) v = -v;

        switch (game_move((int)v)) {
        case MOVE_OK:           return put_board(out, "OK:BOARD:");
        case MOVE_WIN_X:        return put(out, "WIN:X");
        case MOVE_WIN_O:        return put(out, "WIN:O");
        case MOVE_DRAW:         return put(out, "DRAW");
        case MOVE_ERR_NOGAME:   return put(out, "NACK:NOGAME");
        case MOVE_ERR_RANGE:    return put(out, "NACK:RANGE");
        case MOVE_ERR_OCCUPIED: return put(out, "NACK:OCCUPIED");
        }
    }

    return put(out, "NACK:BADCMD");
}
