#ifndef PWM_INPUT_EVALUATOR_H
#define PWM_INPUT_EVALUATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  PWM_INPUT_EVALUATION_WAITING = 0,
  PWM_INPUT_EVALUATION_VALID,
  PWM_INPUT_EVALUATION_TIMED_OUT,
  PWM_INPUT_EVALUATION_OUT_OF_RANGE,
  PWM_INPUT_EVALUATION_INVALID_CONFIG
} PwmInputEvaluationStatus;

typedef struct
{
  uint16_t pulse_width_us;
  uint32_t last_update_ms;
  uint8_t has_sample;
} PwmInputEvaluationInput;

typedef struct
{
  uint16_t raw_pulse_width_us;
  uint16_t pulse_width_us;
  PwmInputEvaluationStatus status;
} PwmInputEvaluationResult;

PwmInputEvaluationResult PwmInputEvaluator_EvaluateConfigured(
  const PwmInputEvaluationInput *input,
  uint32_t now_ms);
const char *PwmInputEvaluator_StatusName(PwmInputEvaluationStatus status);

#ifdef __cplusplus
}
#endif

#endif
