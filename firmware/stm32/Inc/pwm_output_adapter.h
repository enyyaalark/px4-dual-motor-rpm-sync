#ifndef PWM_OUTPUT_ADAPTER_H
#define PWM_OUTPUT_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  PWM_OUTPUT_ADAPTER_VALID = 0,
  PWM_OUTPUT_ADAPTER_INVALID_CONFIG,
  PWM_OUTPUT_ADAPTER_INVALID_REQUEST
} PwmOutputAdapterStatus;

typedef struct
{
  float channel1_requested_us;
  float channel2_requested_us;
} PwmOutputAdapterRequest;

typedef struct
{
  uint16_t minimum_us;
  uint16_t maximum_us;
} PwmOutputAdapterConfig;

typedef struct
{
  uint16_t channel1_us;
  uint16_t channel2_us;
  uint8_t channel1_limited;
  uint8_t channel2_limited;
  PwmOutputAdapterStatus status;
} PwmOutputAdapterResult;

PwmOutputAdapterResult PwmOutputAdapter_Evaluate(
    const PwmOutputAdapterRequest *request,
    const PwmOutputAdapterConfig *config);
PwmOutputAdapterResult PwmOutputAdapter_EvaluateConfigured(
    const PwmOutputAdapterRequest *request);

#ifdef __cplusplus
}
#endif

#endif
