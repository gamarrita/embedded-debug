#ifndef FM_HW_UART_H
#define FM_HW_UART_H

#include <stdbool.h>
#include <stdint.h>

/* Low-level UART for debug transport. */

void FM_HW_UART_Init(void);
bool FM_HW_UART_Transmit(const uint8_t *p_data, uint32_t len, uint32_t timeout_ms);

#endif /* FM_HW_UART_H */
