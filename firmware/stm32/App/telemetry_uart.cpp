#include "telemetry_uart.hpp"

#include <cstdio>

namespace rpm_sync {
namespace {

const char* stateName(AppState state) noexcept {
    switch (state) {
        case AppState::kInit: return "INIT";
        case AppState::kMonitorOnly: return "MONITOR_ONLY";
        case AppState::kSyncControl: return "SYNC_CONTROL";
        case AppState::kBypass: return "BYPASS";
        case AppState::kFault: return "FAULT";
        default: return "UNKNOWN";
    }
}

}  // namespace

const char* telemetryHeader() noexcept {
    return "timestamp_ms,base_pwm_us,rpm1,rpm2,error_rpm,error_percent,"
           "correction_us,pwm1_us,pwm2_us,system_state,fault_flags\n";
}

int formatTelemetry(char* buffer,
                    std::size_t buffer_size,
                    const TelemetrySample& sample) noexcept {
    if ((buffer == nullptr) || (buffer_size == 0U)) {
        return -1;
    }
    return std::snprintf(buffer, buffer_size,
                         "%lu,%u,%.2f,%.2f,%.2f,%.3f,%.2f,%u,%u,%s,0x%04lX\n",
                         static_cast<unsigned long>(sample.timestamp_ms),
                         static_cast<unsigned int>(sample.base_pwm_us),
                         static_cast<double>(sample.rpm1),
                         static_cast<double>(sample.rpm2),
                         static_cast<double>(sample.error_rpm),
                         static_cast<double>(sample.error_percent),
                         static_cast<double>(sample.correction_us),
                         static_cast<unsigned int>(sample.pwm1_us),
                         static_cast<unsigned int>(sample.pwm2_us),
                         stateName(sample.state),
                         static_cast<unsigned long>(sample.fault_flags));
}

}  // namespace rpm_sync
