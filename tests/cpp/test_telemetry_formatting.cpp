#include <array>
#include <cstring>
#include <iostream>
#include <limits>

#include "app_types.hpp"
#include "telemetry_uart.hpp"

namespace {

using rpm_sync::AppState;
using rpm_sync::FaultFlag;
using rpm_sync::TelemetryFormatResult;
using rpm_sync::TelemetryFormatStatus;
using rpm_sync::TelemetrySample;

constexpr char kExpectedHeader[] =
    "timestamp_ms,base_pwm_us,rpm1,rpm2,error_rpm,error_percent,"
    "correction_us,pwm1_us,pwm2_us,system_state,fault_flags\n";
constexpr char kExpectedRow[] =
    "1000,1400,8200.00,8100.00,100.00,1.234,0.00,1400,1400,"
    "MONITOR_ONLY,0x0003\n";

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

TelemetrySample validSample() {
    TelemetrySample sample{};
    sample.timestamp_ms = 1'000U;
    sample.base_pwm_us = 1'400U;
    sample.rpm1 = 8'200.0F;
    sample.rpm2 = 8'100.0F;
    sample.error_rpm = 100.0F;
    sample.error_percent = 1.234F;
    sample.correction_us = 0.0F;
    sample.pwm1_us = 1'400U;
    sample.pwm2_us = 1'400U;
    sample.state = AppState::kMonitorOnly;
    sample.fault_flags =
        rpm_sync::toMask(FaultFlag::kHall1Timeout) |
        rpm_sync::toMask(FaultFlag::kHall2Timeout);
    return sample;
}

bool testHeaderAndExactRow(std::array<char, 192U>& row_buffer) {
    const TelemetryFormatResult result = rpm_sync::formatTelemetry(
        row_buffer.data(), row_buffer.size(), validSample());

    return expect(std::strcmp(rpm_sync::telemetryHeader(), kExpectedHeader) == 0,
                  "telemetry header must preserve the documented 11 fields") &&
           expect(result.status == TelemetryFormatStatus::kOk,
                  "a finite sample must format successfully") &&
           expect(result.length == std::strlen(kExpectedRow),
                  "success length must exclude the null terminator") &&
           expect(std::strcmp(row_buffer.data(), kExpectedRow) == 0,
                  "formatted row must match the documented field order");
}

bool testAllStateNames() {
    constexpr std::array<AppState, 5U> states{
        AppState::kInit,
        AppState::kMonitorOnly,
        AppState::kSyncControl,
        AppState::kBypass,
        AppState::kFault,
    };
    constexpr std::array<const char*, 5U> names{
        "INIT",
        "MONITOR_ONLY",
        "SYNC_CONTROL",
        "BYPASS",
        "FAULT",
    };

    for (std::size_t index = 0U; index < states.size(); ++index) {
        TelemetrySample sample = validSample();
        sample.state = states[index];
        std::array<char, 192U> buffer{};
        const TelemetryFormatResult result = rpm_sync::formatTelemetry(
            buffer.data(), buffer.size(), sample);
        if (!expect(result.status == TelemetryFormatStatus::kOk,
                    "every declared state must format") ||
            !expect(std::strstr(buffer.data(), names[index]) != nullptr,
                    "formatted row must contain the expected state name")) {
            return false;
        }
    }
    return true;
}

bool testInvalidArgumentsAndSmallBuffer() {
    const TelemetrySample sample = validSample();
    const TelemetryFormatResult null_buffer =
        rpm_sync::formatTelemetry(nullptr, 10U, sample);
    std::array<char, 1U> zero_size{{'X'}};
    const TelemetryFormatResult no_capacity =
        rpm_sync::formatTelemetry(zero_size.data(), 0U, sample);
    std::array<char, 16U> small_buffer{};
    small_buffer[0] = 'X';
    const TelemetryFormatResult too_small = rpm_sync::formatTelemetry(
        small_buffer.data(), small_buffer.size(), sample);
    std::array<char, sizeof(kExpectedRow)> exact_buffer{};
    const TelemetryFormatResult exact_fit = rpm_sync::formatTelemetry(
        exact_buffer.data(), exact_buffer.size(), sample);
    std::array<char, sizeof(kExpectedRow) - 1U> one_byte_short{};
    const TelemetryFormatResult short_by_one = rpm_sync::formatTelemetry(
        one_byte_short.data(), one_byte_short.size(), sample);

    return expect(null_buffer.status ==
                      TelemetryFormatStatus::kInvalidArgument,
                  "null buffer must be rejected") &&
           expect(no_capacity.status ==
                      TelemetryFormatStatus::kInvalidArgument,
                  "zero-capacity buffer must be rejected") &&
           expect(zero_size[0] == 'X',
                  "zero capacity must not write through the pointer") &&
           expect(too_small.status ==
                      TelemetryFormatStatus::kBufferTooSmall,
                  "insufficient capacity must be reported") &&
           expect(too_small.length == 0U && small_buffer[0] == '\0',
                  "a truncated row must be made non-sendable") &&
           expect(exact_fit.status == TelemetryFormatStatus::kOk &&
                      std::strcmp(exact_buffer.data(), kExpectedRow) == 0,
                  "a buffer including exactly one null byte must succeed") &&
           expect(short_by_one.status ==
                      TelemetryFormatStatus::kBufferTooSmall &&
                      one_byte_short[0] == '\0',
                  "a buffer without null-terminator capacity must fail closed");
}

bool testInvalidStateAndNonFiniteSamples() {
    TelemetrySample invalid_state = validSample();
    invalid_state.state = static_cast<AppState>(255U);
    std::array<char, 192U> invalid_state_buffer{};
    invalid_state_buffer[0] = 'X';
    const TelemetryFormatResult invalid_state_result =
        rpm_sync::formatTelemetry(invalid_state_buffer.data(),
                                  invalid_state_buffer.size(),
                                  invalid_state);

    TelemetrySample nan_sample = validSample();
    nan_sample.rpm1 = std::numeric_limits<float>::quiet_NaN();
    std::array<char, 192U> nan_buffer{};
    nan_buffer[0] = 'X';
    const TelemetryFormatResult nan_result = rpm_sync::formatTelemetry(
        nan_buffer.data(), nan_buffer.size(), nan_sample);

    TelemetrySample infinite_sample = validSample();
    infinite_sample.error_percent =
        std::numeric_limits<float>::infinity();
    std::array<char, 192U> infinite_buffer{};
    infinite_buffer[0] = 'X';
    const TelemetryFormatResult infinite_result = rpm_sync::formatTelemetry(
        infinite_buffer.data(), infinite_buffer.size(), infinite_sample);

    return expect(invalid_state_result.status ==
                      TelemetryFormatStatus::kInvalidSample,
                  "an undeclared state must be rejected") &&
           expect(nan_result.status == TelemetryFormatStatus::kInvalidSample &&
                      infinite_result.status ==
                          TelemetryFormatStatus::kInvalidSample,
                  "NaN and infinite values must be rejected") &&
           expect(invalid_state_buffer[0] == '\0' &&
                      nan_buffer[0] == '\0' &&
                      infinite_buffer[0] == '\0',
                  "invalid samples must not leave sendable content");
}

}  // namespace

int main() {
    std::array<char, 192U> row_buffer{};
    bool passed = true;
    passed = testHeaderAndExactRow(row_buffer) && passed;
    passed = testAllStateNames() && passed;
    passed = testInvalidArgumentsAndSmallBuffer() && passed;
    passed = testInvalidStateAndNonFiniteSamples() && passed;
    if (!passed) {
        return 1;
    }

    std::cout << "CSV_BEGIN\n"
              << rpm_sync::telemetryHeader()
              << row_buffer.data()
              << "CSV_END\n"
              << "Telemetry formatting host logic tests passed\n";
    return 0;
}
