#ifndef RPM_EVALUATOR_H
#define RPM_EVALUATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
  RPM_EVALUATION_WAITING_FOR_PERIOD = 0,
  RPM_EVALUATION_VALID,
  RPM_EVALUATION_TIMED_OUT,
  RPM_EVALUATION_INVALID_CONFIG,
  RPM_EVALUATION_IMPLAUSIBLE_PULSE
} RpmEvaluationStatus;

typedef struct
{
  uint32_t period_ticks;
  uint32_t last_pulse_ms;
  uint8_t has_pulse;
  uint8_t has_period;
} RpmEvaluationInput;

typedef struct
{
  float timer_hz;
  float pulses_per_revolution;
  uint32_t timeout_ms;
  float maximum_rpm;
} RpmEvaluationConfig;

typedef struct
{
  uint32_t period_ticks;
  uint32_t raw_rpm;
  uint32_t rpm;
  RpmEvaluationStatus status;
} RpmEvaluationResult;

RpmEvaluationResult RpmEvaluator_Evaluate(const RpmEvaluationInput *input,
                                          const RpmEvaluationConfig *config,
                                          uint32_t now_ms);
RpmEvaluationResult RpmEvaluator_EvaluateConfigured(const RpmEvaluationInput *input,
                                                    uint32_t now_ms);
const char *RpmEvaluator_StatusName(RpmEvaluationStatus status);

#ifdef __cplusplus
}
#endif

#endif
