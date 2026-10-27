/* game.c - authoritative Tic-Tac-Toe state. Plain C, no HAL. */
#include "game.h"

static char      board[10];
static char      turn;
static GameState state;

static const unsigned char LINES[8][3] = {
    {0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}
};

static void clear_board(void)
{
    for (int i = 0; i < 9; i++) board[i] = '.';
    board[9] = '\0';
    turn = 'X';
}

void game_init(void)
{
    clear_board();
    state = GS_IDLE;
}

void game_new(void)
{
    clear_board();
    state = GS_IN_PROGRESS;
}

void game_resign(void)
{
    if (state == GS_IN_PROGRESS) state = GS_GAME_OVER;
}

MoveResult game_move(int index)
{
    /* All checks come first: a rejected move must not touch any state. */
    if (state != GS_IN_PROGRESS) return MOVE_ERR_NOGAME;
    if (index < 0 || index > 8)  return MOVE_ERR_RANGE;
    if (board[index] != '.')     return MOVE_ERR_OCCUPIED;

    board[index] = turn;

    for (int l = 0; l < 8; l++) {
        if (board[LINES[l][0]] == turn &&
            board[LINES[l][1]] == turn &&
            board[LINES[l][2]] == turn) {
            state = GS_GAME_OVER;
            return (turn == 'X') ? MOVE_WIN_X : MOVE_WIN_O;
        }
    }

    int full = 1;
    for (int i = 0; i < 9; i++) if (board[i] == '.') full = 0;
    if (full) {
        state = GS_GAME_OVER;
        return MOVE_DRAW;
    }

    turn = (turn == 'X') ? 'O' : 'X';
    return MOVE_OK;
}

GameState   game_state(void) { return state; }
char        game_turn(void)  { return turn; }
const char *game_board(void) { return board; }
