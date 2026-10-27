/* game.h - authoritative Tic-Tac-Toe state. Plain C, no HAL. */
#ifndef GAME_H
#define GAME_H

typedef enum { GS_IDLE = 0, GS_IN_PROGRESS, GS_GAME_OVER } GameState;

typedef enum {
    MOVE_OK = 0,        /* legal, game continues       */
    MOVE_WIN_X,         /* legal, X wins (game over)   */
    MOVE_WIN_O,         /* legal, O wins (game over)   */
    MOVE_DRAW,          /* legal, board full (game over) */
    MOVE_ERR_NOGAME,    /* not IN_PROGRESS             */
    MOVE_ERR_RANGE,     /* index not 0..8              */
    MOVE_ERR_OCCUPIED   /* cell already taken          */
} MoveResult;

void        game_init(void);          /* back to IDLE, empty board */
void        game_new(void);           /* empty board, X to move, IN_PROGRESS */
void        game_resign(void);        /* IN_PROGRESS -> GAME_OVER */
MoveResult  game_move(int index);     /* rejected moves change nothing */
GameState   game_state(void);
char        game_turn(void);          /* 'X' or 'O' */
const char *game_board(void);         /* 9 chars + NUL, '.' = empty */

#endif
