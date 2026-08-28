#ifndef TELEMETRY_UART_H
#define TELEMETRY_UART_H

#include <stddef.h>
#include "app_types.h"

const char *telemetry_uart_header(void);
int telemetry_uart_format(char *buffer,
                          size_t buffer_size,
                          const app_telemetry_sample_t *sample);

#endif
