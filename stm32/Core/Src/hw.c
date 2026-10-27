/* hw.c - on-board button and LEDs (see hw.h). Runs in the main loop only,
 * so it can change the game state without any locking. */
#include "hw.h"
#include "game.h"
#include "stm32f0xx_hal.h"

#define LED_ACTIVE_HIGH  1             /* set to 0 if both LEDs are lit while IDLE */
#define LED_PORT         GPIOB
#define LED_X_PIN        GPIO_PIN_0
#define LED_O_PIN        GPIO_PIN_1
#define BTN_PORT         GPIOA
#define BTN_PIN          GPIO_PIN_0
#define BLINK_MS         200u          /* game-over blink half-period */
#define DEBOUNCE_MS      30u

static GPIO_PinState idle_level;       /* button level when NOT pressed (measured at start-up) */
static GPIO_PinState stable_level;     /* debounced level */
static GPIO_PinState last_raw;
static uint32_t      raw_changed_ms;

static void led(uint16_t pin, int on)
{
    int level_high = ((on != 0) == (LED_ACTIVE_HIGH != 0));
    HAL_GPIO_WritePin(LED_PORT, pin, level_high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void hw_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    g.Pin   = LED_X_PIN | LED_O_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &g);
    led(LED_X_PIN, 0);
    led(LED_O_PIN, 0);

    g.Pin  = BTN_PIN;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BTN_PORT, &g);

    HAL_Delay(5);                                   /* let the pin settle */
    idle_level   = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);   /* works for active-low or active-high */
    stable_level = idle_level;
    last_raw     = idle_level;
    raw_changed_ms = HAL_GetTick();
}

void hw_poll(void)
{
    uint32_t now = HAL_GetTick();

    /* --- button: debounce, act on the press --- */
    GPIO_PinState raw = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);
    if (raw != last_raw) {
        last_raw = raw;
        raw_changed_ms = now;
    } else if (raw != stable_level && (int32_t)(now - raw_changed_ms) >= (int32_t)DEBOUNCE_MS) {
        stable_level = raw;
        if (stable_level != idle_level) {
            game_new();                             /* local reset: fresh game, X to move */
        }
    }

    /* --- LEDs follow the authoritative game state --- */
    switch (game_state()) {
    case GS_IN_PROGRESS:
        led(LED_X_PIN, game_turn() == 'X');
        led(LED_O_PIN, game_turn() == 'O');
        break;
    case GS_GAME_OVER: {
        int on = (int)((now / BLINK_MS) & 1u);
        led(LED_X_PIN, on);
        led(LED_O_PIN, on);
        break;
    }
    default:                                        /* IDLE */
        led(LED_X_PIN, 0);
        led(LED_O_PIN, 0);
        break;
    }
}
