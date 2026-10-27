/* PC test of game + protocol + framer (no board, no HAL).
 * Build/run:  make -C host_tests test */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "game.h"
#include "protocol.h"
#include "framer.h"

static uint32_t now;           /* simulated milliseconds */
static int fails, checks;

#define CHECK(cond) do { checks++; if (!(cond)) { fails++; \
    printf("FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

/* Feed raw bytes (like the UART ISR would) */
static void feed(const char *s, size_t n) { for (size_t i = 0; i < n; i++) framer_feed((uint8_t)s[i], now++); }

/* Process every queued frame, return the LAST reply (and count them) */
static char last[64]; static int replies;
static void drain(void)
{
    Frame f; char out[PROTOCOL_REPLY_MAX + 2];
    while (framer_get(&f)) {
        protocol_handle(f.text, f.len, f.err, out);
        strcpy(last, out); replies++;
    }
}
static const char *send_line(const char *cmd)       /* cmd + "\n", expects exactly one reply */
{
    replies = 0; last[0] = 0;
    feed(cmd, strlen(cmd)); feed("\n", 1);
    drain();
    return last;
}
#define EQ(a, b) (strcmp((a), (b)) == 0)

static void fresh(void) { game_init(); framer_init(); now = 1000; replies = 0; }
static void start(void) { send_line("CONNECT"); send_line("NEW"); }

int main(void)
{
    /* --- same cases as tests/test_game.py ------------------------------- */
    fresh();
    CHECK(EQ(send_line("CONNECT"), "OK"));
    CHECK(EQ(send_line("NEW"), "OK:BOARD:........."));

    fresh(); start();
    CHECK(EQ(send_line("MOVE:4"), "OK:BOARD:....X...."));
    CHECK(EQ(send_line("STATE"), "BOARD:....X...."));

    fresh(); start(); send_line("MOVE:4");
    CHECK(EQ(send_line("MOVE:4"), "NACK:OCCUPIED"));
    CHECK(EQ(send_line("STATE"), "BOARD:....X...."));

    fresh(); start();
    CHECK(EQ(send_line("MOVE:9"), "NACK:RANGE"));
    CHECK(EQ(send_line("MOVE:-1"), "NACK:RANGE"));
    CHECK(EQ(send_line("MOVE:99999999999999"), "NACK:RANGE"));

    fresh(); start();
    { int m[] = {0,3,1,4,2}; const char *r = "";
      for (int i = 0; i < 5; i++) { char b[16]; sprintf(b, "MOVE:%d", m[i]); r = send_line(b); }
      CHECK(EQ(r, "WIN:X")); }
    CHECK(EQ(send_line("MOVE:5"), "NACK:NOGAME"));
    CHECK(EQ(send_line("STATE"), "BOARD:XXXOO...."));

    fresh(); start();
    { int m[] = {0,1,2,4,3,5,7,6,8}; const char *r = "";
      for (int i = 0; i < 9; i++) { char b[16]; sprintf(b, "MOVE:%d", m[i]); r = send_line(b); }
      CHECK(EQ(r, "DRAW")); }

    fresh(); start();
    CHECK(EQ(send_line("HELLO"), "NACK:BADCMD"));
    CHECK(EQ(send_line("MOVE:abc"), "NACK:BADCMD"));
    CHECK(EQ(send_line("MOVE:"), "NACK:BADCMD"));
    CHECK(EQ(send_line("move:4"), "NACK:BADCMD"));
    CHECK(EQ(send_line("MOVE:4x"), "NACK:BADCMD"));
    CHECK(EQ(send_line("STATE"), "BOARD:........."));

    fresh();
    CHECK(EQ(send_line("CONNECT"), "OK"));
    CHECK(EQ(send_line("MOVE:0"), "NACK:NOGAME"));
    CHECK(EQ(send_line("STATE"), "NACK:NOGAME"));

    /* Real board keeps state between tests: CONNECT must reset it */
    fresh(); start(); send_line("MOVE:4");
    CHECK(EQ(send_line("CONNECT"), "OK"));
    CHECK(EQ(send_line("NEW"), "OK:BOARD:........."));

    /* --- state machine ---------------------------------------------------- */
    fresh(); send_line("CONNECT"); send_line("NEW");
    CHECK(EQ(send_line("NEW"), "NACK:BADCMD"));          /* NEW not allowed IN_PROGRESS */
    CHECK(EQ(send_line("RESIGN"), "OK:GAMEOVER"));
    CHECK(EQ(send_line("MOVE:0"), "NACK:NOGAME"));
    CHECK(EQ(send_line("STATE"), "BOARD:........."));    /* allowed in GAME_OVER */
    CHECK(EQ(send_line("RESIGN"), "NACK:NOGAME"));
    CHECK(EQ(send_line("NEW"), "OK:BOARD:........."));   /* NEW allowed in GAME_OVER */
    CHECK(EQ(send_line("DISCONNECT"), "OK"));
    CHECK(EQ(send_line("STATE"), "NACK:NOGAME"));

    /* turn alternates and rejected moves do not change it */
    fresh(); start();
    send_line("MOVE:0");                                 /* X */
    send_line("MOVE:0");                                 /* rejected */
    CHECK(EQ(send_line("MOVE:1"), "OK:BOARD:XO......."));/* must be O's move */

    /* --- framing ---------------------------------------------------------- */
    fresh(); start();
    CHECK(EQ(send_line("MOVE:4\r"), "OK:BOARD:....X...."));       /* CRLF */

    /* 31 chars + newline is a valid (but unknown) command: BADCMD, 1 reply */
    { char b[40]; memset(b, 'A', 31); b[31] = 0;
      fresh(); start();
      CHECK(EQ(send_line(b), "NACK:BADCMD")); CHECK(replies == 1); }

    /* exactly 32 chars: too long, ONE reply, next command works */
    { char b[40]; memset(b, 'A', 32); b[32] = 0;
      fresh(); start();
      CHECK(EQ(send_line(b), "NACK:BADCMD")); CHECK(replies == 1);
      CHECK(EQ(send_line("STATE"), "BOARD:.........")); }

    /* 100 chars: ONE reply */
    { char b[120]; memset(b, 'A', 100); b[100] = 0;
      fresh(); start();
      CHECK(EQ(send_line(b), "NACK:BADCMD")); CHECK(replies == 1);
      CHECK(EQ(send_line("STATE"), "BOARD:.........")); }

    /* a long line whose tail looks like a command is NOT executed */
    { char b[80]; memset(b, 'A', 40); strcpy(b + 40, "MOVE:4"); b[46] = 0;
      fresh(); start();
      CHECK(EQ(send_line(b), "NACK:BADCMD")); CHECK(replies == 1);
      CHECK(EQ(send_line("STATE"), "BOARD:.........")); }

    /* garbage bytes, and an embedded NUL must not truncate to a valid command */
    fresh(); start();
    CHECK(EQ(send_line("\x01\x02\x03"), "NACK:BADCMD"));
    replies = 0; feed("MOVE:4\0junk\n", 12); drain();
    CHECK(EQ(last, "NACK:BADCMD")); CHECK(replies == 1);
    CHECK(EQ(send_line("STATE"), "BOARD:........."));
    CHECK(EQ(send_line(""), "NACK:BADCMD"));                      /* empty line */

    /* incomplete frame times out; fragment afterwards is a NEW frame */
    fresh(); start();
    feed("MOVE:", 5);
    now += 600; framer_check_timeout(now);
    CHECK(EQ(send_line("4"), "NACK:BADCMD"));                     /* late "4" is bad */
    CHECK(EQ(send_line("STATE"), "BOARD:........."));

    /* incomplete frame, NOT yet timed out: the rest completes it normally */
    fresh(); start();
    feed("MOVE:", 5); now += 100; framer_check_timeout(now);
    CHECK(EQ(send_line("4"), "OK:BOARD:....X...."));

    /* oversized with NO newline: discard ends after the 500 ms silence */
    { char b[60]; memset(b, 'A', 50); b[50] = 0;
      fresh(); start(); replies = 0;
      feed(b, 50); drain();
      CHECK(replies == 1 && EQ(last, "NACK:BADCMD"));
      now += 600; framer_check_timeout(now);
      CHECK(EQ(send_line("STATE"), "BOARD:.........")); }

    /* ...but if the silence is shorter, the next line is still being discarded */
    { char b[60]; memset(b, 'A', 50); b[50] = 0;
      fresh(); start();
      feed(b, 50); drain();
      now += 100; framer_check_timeout(now);
      replies = 0; feed("STATE\n", 6); drain();
      CHECK(replies == 0); }                                     /* swallowed up to its newline */

    /* UART hardware error: partial frame dropped, skip to newline, then recover */
    fresh(); start();
    feed("MOV", 3); framer_stream_error();
    replies = 0; feed("E:4\n", 4); drain();
    CHECK(replies == 0);
    CHECK(EQ(send_line("STATE"), "BOARD:........."));

    /* --- queue: 3 usable, FIFO, wraparound, never overwrites ---------------- */
    fresh(); start();
    replies = 0; feed("MOVE:0\nMOVE:1\nMOVE:2\nMOVE:3\n", 28);    /* 4th must be dropped */
    CHECK(framer_dropped() == 1);
    { Frame f; char out[PROTOCOL_REPLY_MAX + 2]; const char *want[3] = {"OK:BOARD:X........","OK:BOARD:XO.......","OK:BOARD:XOX......"};
      for (int i = 0; i < 3; i++) { CHECK(framer_get(&f)); protocol_handle(f.text, f.len, f.err, out); CHECK(EQ(out, want[i])); }
      CHECK(!framer_get(&f)); }
    /* wraparound: many rounds through the 4 slots */
    fresh(); start();
    for (int i = 0; i < 50; i++) { CHECK(EQ(send_line("STATE"), "BOARD:.........")); }
    CHECK(framer_dropped() == 0);

    /* move validity is judged when processed, not when received */
    fresh(); start();
    feed("MOVE:4\nMOVE:4\n", 14);
    { Frame f; char o1[32], o2[32]; framer_get(&f); protocol_handle(f.text, f.len, f.err, o1);
      framer_get(&f); protocol_handle(f.text, f.len, f.err, o2);
      CHECK(EQ(o1, "OK:BOARD:....X....")); CHECK(EQ(o2, "NACK:OCCUPIED")); }

    printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
