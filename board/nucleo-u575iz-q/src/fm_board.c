/**
 * @file    fm_board.c
 * @brief   Board adaptation layer for NUCLEO-U575ZI-Q.
 *
 * @details
 *  - Owns hardware init (GPIO for LEDs/jumpers).
 *  - Provides delay and jumper read helpers.
 *  - Receives HAL callbacks and forwards to app.
 */

#include "fm_board.h"

#include "fm_main.h"
#include "main.h"

/* Private Defines */
#define LED_INIT_SPEED      GPIO_SPEED_FREQ_LOW
#define LED_INIT_MODE       GPIO_MODE_OUTPUT_PP
#define LED_INIT_PULL       GPIO_NOPULL

/* Private Prototypes */
static void fm_board_enable_gpio_clock(GPIO_TypeDef *port);
static bool fm_board_read_jumper(GPIO_TypeDef *port, uint16_t pin);

/* Private Bodies */
static void fm_board_enable_gpio_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
    else if (port == GPIOB)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
    else if (port == GPIOC)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    else if (port == GPIOD)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }
    else if (port == GPIOE)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
    else if (port == GPIOF)
    {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    }
    else if (port == GPIOG)
    {
        __HAL_RCC_GPIOG_CLK_ENABLE();
    }
    else if (port == GPIOH)
    {
        __HAL_RCC_GPIOH_CLK_ENABLE();
    }
}

static bool fm_board_read_jumper(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    bool enabled;

    fm_board_enable_gpio_clock(port);

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    enabled = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET);

    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    return enabled;
}

/* Public Bodies */
void FM_BOARD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    fm_board_enable_gpio_clock(LED_ERROR_GPIO_Port);
    fm_board_enable_gpio_clock(LED_RUN_GPIO_Port);
    fm_board_enable_gpio_clock(LED_SIGNAL_GPIO_Port);
    fm_board_enable_gpio_clock(DBG_MSG_EN_GPIO_Port);
    fm_board_enable_gpio_clock(DBG_LED_EN_GPIO_Port);

    GPIO_InitStruct.Mode = LED_INIT_MODE;
    GPIO_InitStruct.Pull = LED_INIT_PULL;
    GPIO_InitStruct.Speed = LED_INIT_SPEED;

    GPIO_InitStruct.Pin = LED_ERROR_Pin;
    HAL_GPIO_Init(LED_ERROR_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = LED_RUN_Pin;
    HAL_GPIO_Init(LED_RUN_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = LED_SIGNAL_Pin;
    HAL_GPIO_Init(LED_SIGNAL_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_SIGNAL_GPIO_Port, LED_SIGNAL_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = DBG_MSG_EN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DBG_MSG_EN_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DBG_LED_EN_Pin;
    HAL_GPIO_Init(DBG_LED_EN_GPIO_Port, &GPIO_InitStruct);
}

void FM_BOARD_DelayMs(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

bool FM_BOARD_IsDbgLedEnabled(void)
{
    return fm_board_read_jumper(DBG_LED_EN_GPIO_Port, DBG_LED_EN_Pin);
}

bool FM_BOARD_IsDbgMsgEnabled(void)
{
    return fm_board_read_jumper(DBG_MSG_EN_GPIO_Port, DBG_MSG_EN_Pin);
}

void FM_BOARD_LedErrorOn(void)
{
    HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_SET);
}

void FM_BOARD_LedErrorOff(void)
{
    HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_RESET);
}

void FM_BOARD_LedRunOn(void)
{
    HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_SET);
}

void FM_BOARD_LedRunOff(void)
{
    HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_RESET);
}

void FM_BOARD_LedSignalOn(void)
{
    HAL_GPIO_WritePin(LED_SIGNAL_GPIO_Port, LED_SIGNAL_Pin, GPIO_PIN_SET);
}

void FM_BOARD_LedSignalOff(void)
{
    HAL_GPIO_WritePin(LED_SIGNAL_GPIO_Port, LED_SIGNAL_Pin, GPIO_PIN_RESET);
}

/* HAL Callbacks ------------------------------------------------------------ */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    UNUSED(hrtc);
    FM_MAIN_OnRtcWakeup();
}

/* Interrupts */
/* (none) */