#include "fm_board_gpio.h"

#include "main.h"

/* Private Prototypes */
static void fm_board_gpio_enable_clock(GPIO_TypeDef *port);
static bool fm_board_gpio_read_jumper(GPIO_TypeDef *port, uint16_t pin);

/* Private Bodies */
static void fm_board_gpio_enable_clock(GPIO_TypeDef *port)
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

static bool fm_board_gpio_read_jumper(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    bool enabled;

    fm_board_gpio_enable_clock(port);

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
void FM_BOARD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    fm_board_gpio_enable_clock(LED_ERROR_GPIO_Port);
    fm_board_gpio_enable_clock(LED_RUN_GPIO_Port);
    fm_board_gpio_enable_clock(LED_SIGNAL_GPIO_Port);
    fm_board_gpio_enable_clock(DBG_MSG_EN_GPIO_Port);
    fm_board_gpio_enable_clock(DBG_LED_EN_GPIO_Port);

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = LED_ERROR_Pin;
    HAL_GPIO_Init(LED_ERROR_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = LED_RUN_Pin;
    HAL_GPIO_Init(LED_RUN_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = LED_SIGNAL_Pin;
    HAL_GPIO_Init(LED_SIGNAL_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_SIGNAL_GPIO_Port, LED_SIGNAL_Pin, GPIO_PIN_RESET);

    /* Leave jumpers in lowest-power state when not sampling. */
    GPIO_InitStruct.Pin = DBG_MSG_EN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DBG_MSG_EN_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DBG_LED_EN_Pin;
    HAL_GPIO_Init(DBG_LED_EN_GPIO_Port, &GPIO_InitStruct);
}

bool FM_BOARD_GPIO_IsDbgLedEnabled(void)
{
    return fm_board_gpio_read_jumper(DBG_LED_EN_GPIO_Port, DBG_LED_EN_Pin);
}

bool FM_BOARD_GPIO_IsDbgMsgEnabled(void)
{
    return fm_board_gpio_read_jumper(DBG_MSG_EN_GPIO_Port, DBG_MSG_EN_Pin);
}

void FM_BOARD_GPIO_LedErrorOn(void)
{
    HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_SET);
}

void FM_BOARD_GPIO_LedErrorOff(void)
{
    HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_RESET);
}

void FM_BOARD_GPIO_LedRunOn(void)
{
    HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_SET);
}

void FM_BOARD_GPIO_LedRunOff(void)
{
    HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_RESET);
}

void FM_BOARD_GPIO_LedSignalOn(void)
{
    HAL_GPIO_WritePin(LED_SIGNAL_GPIO_Port, LED_SIGNAL_Pin, GPIO_PIN_SET);
}

void FM_BOARD_GPIO_LedSignalOff(void)
{
    HAL_GPIO_WritePin(LED_SIGNAL_GPIO_Port, LED_SIGNAL_Pin, GPIO_PIN_RESET);
}