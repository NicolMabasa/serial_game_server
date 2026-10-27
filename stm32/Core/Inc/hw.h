/* hw.h - on-board button and LEDs.
 *   PA0        : button  -> press = local reset / new game
 *   PB0 (LED X): on while it is X's turn
 *   PB1 (LED O): on while it is O's turn
 *   game over  : both LEDs blink together;  IDLE: both off */
#ifndef HW_H
#define HW_H

void hw_init(void);   /* once, after HAL_Init() */
void hw_poll(void);   /* every main-loop pass: debounce button, drive LEDs */

#endif
