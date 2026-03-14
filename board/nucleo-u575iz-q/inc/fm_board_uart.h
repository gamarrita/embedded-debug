#ifndef FM_BOARD_UART_H
#define FM_BOARD_UART_H

#include <stdbool.h>
#include <stdint.h>

void FM_BOARD_UART_Init(void);
bool FM_BOARD_UART_Transmit(const uint8_t *p_data, uint32_t len, uint32_t timeout_ms);

#endif /* FM_BOARD_UART_H */