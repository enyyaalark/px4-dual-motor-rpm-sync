#!/usr/bin/env python3
"""Capture and compare bounded bench runs for baseline and P-sync evidence."""

from __future__ import annotations

import argparse
import statistics
import sys
import time
from dataclasses import dataclass
from pathlib import Path

_TOOLS_DIR = Path(__file__).resolve().parent
if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))


BAUD_RATE = 115200
CAPTURE_SOURCE = "rpm_sync_capture"
PWM_INPUT_SOURCE = "rpm_sync_pwm_input"
CTRL_SOURCE = "rpm_sync_ctrl"
CTRL_VERSION = "v1"
CTRL_KEYS = (
    "t_ms",
    "base_us",
    "pwm1_us",
    "pwm2_us",
    "rpm1",
    "rpm2",
    "error_rpm",
    "error_percent",
    "correction_us",
    "select",
    "state",
    "fault",
)


@dataclass(frozen=True)
class CtrlSample:
    timestamp_ms: int
    base_us: int
    pwm1_us: int
    pwm2_us: int
    rpm1: int
    rpm2: int
    error_rpm: int
    error_percent: int
    correction_us: int
    select: int
    state: int
    fault: int


@dataclass(frozen=True)
class BenchRun:
    path: Path
    ctrl_samples: list[CtrlSample]
    capture_total: int
    capture_both_valid: int
    pwm_input_total: int
    pwm_input_both_valid: int
    raw_line_count: int


@dataclass(frozen=True)
class BenchMetrics:
    ctrl_count: int
    valid_ctrl_count: int
    median_abs_error_rpm: float | None
    median_abs_error_percent: float | None
    max_rpm: int | None
    selected_corrected_count: int
    corrected_pwm1_min: int | None
    corrected_pwm1_max: int | None
    corrected_pwm2_min: int | None
    corrected_pwm2_max: int | None
    states: set[int]
    fault_flags: set[int]
    capture_both_valid_rate: float | None
    pwm_input_both_valid_rate: float | None


def _parse_uint(value: str, name: str, maximum: int) -> int:
    if not value.isdecimal():
        raise ValueError(f"{name} must be an unsigned decimal integer")
    parsed = int(value)
    if parsed < 0 or parsed > maximum:
        raise ValueError(f"{name} is out of range")
    return parsed


def _parse_signed(value: str, name: str, minimum: int, maximum: int) -> int:
    if not value.lstrip("-").isdecimal():
        raise ValueError(f"{name} must be a decimal integer")
    parsed = int(value)
    if parsed < minimum or parsed > maximum:
        raise ValueError(f"{name} is out of range")
    return parsed


def _parse_hex_uint(value: str, name: str, maximum: int) -> int:
    if not value.lower().startswith("0x"):
        raise ValueError(f"{name} must be a hexadecimal integer")
    digits = value[2:]
    if not digits or any(character not in "0123456789abcdef" for character in digits.lower()):
        raise ValueError(f"{name} must be a hexadecimal integer")
    parsed = int(digits, 16)
    if parsed < 0 or parsed > maximum:
        raise ValueError(f"{name} is out of range")
    return parsed


def parse_ctrl_line(line: str) -> CtrlSample:
    """Parse one exact ``rpm_sync_ctrl,v1`` line or raise ValueError."""
    parts = line.strip().split(",")
    if len(parts) < 2 or parts[:2] != [CTRL_SOURCE, CTRL_VERSION]:
        raise ValueError("unexpected control source or version")
    if len(parts) != len(CTRL_KEYS) + 2:
        raise ValueError("control line contains the wrong number of fields")

    fields: dict[str, str] = {}
    for part in parts[2:]:
        if "=" not in part:
            raise ValueError("control fields must use key=value")
        key, value = part.split("=", 1)
        if key in fields:
            raise ValueError(f"duplicate control field: {key}")
        fields[key] = value
    if tuple(fields) != CTRL_KEYS:
        raise ValueError("unexpected control field order or names")

    return CtrlSample(
        timestamp_ms=_parse_uint(fields["t_ms"], "t_ms", (1 << 32) - 1),
        base_us=_parse_uint(fields["base_us"], "base_us", (1 << 16) - 1),
        pwm1_us=_parse_uint(fields["pwm1_us"], "pwm1_us", (1 << 16) - 1),
        pwm2_us=_parse_uint(fields["pwm2_us"], "pwm2_us", (1 << 16) - 1),
        rpm1=_parse_uint(fields["rpm1"], "rpm1", (1 << 32) - 1),
        rpm2=_parse_uint(fields["rpm2"], "rpm2", (1 << 32) - 1),
        error_rpm=_parse_signed(fields["error_rpm"], "error_rpm", -(1 << 31), (1 << 31) - 1),
        error_percent=_parse_signed(
            fields["error_percent"], "error_percent", -(1 << 31), (1 << 31) - 1
        ),
        correction_us=_parse_signed(
            fields["correction_us"], "correction_us", -(1 << 31), (1 << 31) - 1
        ),
        select=_parse_uint(fields["select"], "select", 1),
        state=_parse_uint(fields["state"], "state", 255),
        fault=_parse_hex_uint(fields["fault"], "fault", (1 << 32) - 1),
    )


def _try_parse_capture(line: str) -> bool:
    try:
        import check_hall_capture_uart  # type: ignore

        sample = check_hall_capture_uart.parse_capture_line(line)
    except (ImportError, ValueError):
        return False
    if all(status is not None for status in sample.status):
        return sample.status == ("VALID", "VALID")
    return bool(sample.valid[0] and sample.valid[1])


def _try_parse_pwm_input(line: str) -> bool:
    try:
        import check_pwm_input_uart  # type: ignore

        sample = check_pwm_input_uart.parse_pwm_input_line(line)
    except (ImportError, ValueError):
        return False
    return sample.status == ("VALID", "VALID")


def analyze_lines(path: Path, lines: list[str]) -> BenchRun:
    ctrl_samples: list[CtrlSample] = []
    capture_total = 0
    capture_both_valid = 0
    pwm_input_total = 0
    pwm_input_both_valid = 0

    for raw_line in lines:
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith(CAPTURE_SOURCE):
            capture_total += 1
            if _try_parse_capture(line):
                capture_both_valid += 1
        elif line.startswith(PWM_INPUT_SOURCE):
            pwm_input_total += 1
            if _try_parse_pwm_input(line):
                pwm_input_both_valid += 1
        elif line.startswith(CTRL_SOURCE):
            try:
                ctrl_samples.append(parse_ctrl_line(line))
            except ValueError:
                continue

    return BenchRun(
        path=path,
        ctrl_samples=ctrl_samples,
        capture_total=capture_total,
        capture_both_valid=capture_both_valid,
        pwm_input_total=pwm_input_total,
        pwm_input_both_valid=pwm_input_both_valid,
        raw_line_count=len(lines),
    )


def analyze_file(path: Path) -> BenchRun:
    lines = path.read_text(encoding="utf-8").splitlines()
    return analyze_lines(path, lines)


def _rate(numerator: int, denominator: int) -> float | None:
    if denominator <= 0:
        return None
    return numerator / denominator


def metrics(run: BenchRun) -> BenchMetrics:
    valid_ctrl = [sample for sample in run.ctrl_samples if sample.rpm1 > 0 and sample.rpm2 > 0]
    median_abs_error_rpm: float | None = None
    median_abs_error_percent: float | None = None
    max_rpm: int | None = None
    if valid_ctrl:
        median_abs_error_rpm = float(statistics.median(abs(s.error_rpm) for s in valid_ctrl))
        median_abs_error_percent = float(
            statistics.median(abs(s.error_percent) for s in valid_ctrl)
        )
        max_rpm = max(max(s.rpm1, s.rpm2) for s in valid_ctrl)

    corrected = [s for s in valid_ctrl if s.select == 1]
    return BenchMetrics(
        ctrl_count=len(run.ctrl_samples),
        valid_ctrl_count=len(valid_ctrl),
        median_abs_error_rpm=median_abs_error_rpm,
        median_abs_error_percent=median_abs_error_percent,
        max_rpm=max_rpm,
        selected_corrected_count=len(corrected),
        corrected_pwm1_min=min((s.pwm1_us for s in corrected), default=None),
        corrected_pwm1_max=max((s.pwm1_us for s in corrected), default=None),
        corrected_pwm2_min=min((s.pwm2_us for s in corrected), default=None),
        corrected_pwm2_max=max((s.pwm2_us for s in corrected), default=None),
        states={s.state for s in run.ctrl_samples},
        fault_flags={s.fault for s in run.ctrl_samples},
        capture_both_valid_rate=_rate(run.capture_both_valid, run.capture_total),
        pwm_input_both_valid_rate=_rate(run.pwm_input_both_valid, run.pwm_input_total),
    )


def _print_metrics(label: str, run: BenchRun) -> BenchMetrics:
    run_metrics = metrics(run)
    print(f"{label}: {run.path}")
    print(f"  raw lines: {run.raw_line_count}")
    print(f"  ctrl samples: {run_metrics.ctrl_count}")
    print(f"  ctrl samples with positive dual RPM: {run_metrics.valid_ctrl_count}")
    print(
        f"  median abs error_rpm: "
        f"{run_metrics.median_abs_error_rpm:.1f}" if run_metrics.median_abs_error_rpm is not None else "  median abs error_rpm: N/A"
    )
    print(
        f"  median abs error_percent: "
        f"{run_metrics.median_abs_error_percent:.2f}%" if run_metrics.median_abs_error_percent is not None else "  median abs error_percent: N/A"
    )
    print(
        f"  max RPM: {run_metrics.max_rpm}" if run_metrics.max_rpm is not None else "  max RPM: N/A"
    )
    print(
        f"  capture both-valid rate: "
        f"{run_metrics.capture_both_valid_rate:.3f}" if run_metrics.capture_both_valid_rate is not None else "  capture both-valid rate: N/A"
    )
    print(
        f"  PWM-input both-valid rate: "
        f"{run_metrics.pwm_input_both_valid_rate:.3f}" if run_metrics.pwm_input_both_valid_rate is not None else "  PWM-input both-valid rate: N/A"
    )
    print(
        f"  corrected selection count: {run_metrics.selected_corrected_count}; "
        f"states={sorted(run_metrics.states)}; faults={[hex(f) for f in sorted(run_metrics.fault_flags)]}"
    )
    if run_metrics.corrected_pwm1_min is not None:
        print(
            f"  corrected pwm1 range: "
            f"{run_metrics.corrected_pwm1_min}..{run_metrics.corrected_pwm1_max} us"
        )
        print(
            f"  corrected pwm2 range: "
            f"{run_metrics.corrected_pwm2_min}..{run_metrics.corrected_pwm2_max} us"
        )
    return run_metrics


def compare_runs(baseline_path: Path, sync_path: Path) -> int:
    baseline = analyze_file(baseline_path)
    sync = analyze_file(sync_path)
    baseline_metrics = _print_metrics("baseline", baseline)
    sync_metrics = _print_metrics("sync", sync)

    if (
        baseline_metrics.median_abs_error_rpm is None
        or sync_metrics.median_abs_error_rpm is None
    ):
        print("comparison failed: one or both runs have no positive dual-RPM ctrl samples")
        return 1

    error_decreased = (
        sync_metrics.median_abs_error_rpm < baseline_metrics.median_abs_error_rpm
    )
    percent_decreased = (
        sync_metrics.median_abs_error_percent < baseline_metrics.median_abs_error_percent
    )
    print(
        f"baseline median abs error_rpm: {baseline_metrics.median_abs_error_rpm:.1f}; "
        f"sync median abs error_rpm: {sync_metrics.median_abs_error_rpm:.1f}"
    )
    print(
        f"baseline median abs error_percent: {baseline_metrics.median_abs_error_percent:.2f}%; "
        f"sync median abs error_percent: {sync_metrics.median_abs_error_percent:.2f}%"
    )
    if error_decreased and percent_decreased:
        print("verdict: IMPROVED")
        return 0
    print("verdict: NO_IMPROVEMENT")
    return 1


def _write_raw_record(path: Path, lines: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8", newline="") as output:
        for line in lines:
            output.write(line.rstrip("\r\n") + "\n")


def capture_raw(port: str, duration_s: float) -> list[str]:
    try:
        import serial
    except ImportError as error:
        raise RuntimeError(
            "pyserial is missing; install tools/requirements.txt in the project venv"
        ) from error

    lines: list[str] = []
    deadline = time.monotonic() + duration_s
    with serial.Serial(
        port=port,
        baudrate=BAUD_RATE,
        timeout=min(1.0, duration_s),
        xonxoff=False,
        rtscts=False,
        dsrdtr=False,
    ) as connection:
        while time.monotonic() < deadline:
            raw_line = connection.readline()
            if not raw_line:
                continue
            lines.append(raw_line.decode("utf-8", errors="replace").rstrip("\r\n"))
    return lines


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    capture = subparsers.add_parser("capture", help="capture a bounded raw UART bench run")
    capture.add_argument("--port", required=True, help="serial device selected locally")
    capture.add_argument("--duration-s", type=float, required=True)
    capture.add_argument("--output", type=Path, required=True)
    capture.add_argument("--analyze", action="store_true")

    compare = subparsers.add_parser("compare", help="compare baseline and P-sync raw logs")
    compare.add_argument("--baseline", type=Path, required=True)
    compare.add_argument("--sync", type=Path, required=True)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.command == "capture":
        if args.duration_s <= 0:
            print("duration must be positive", file=sys.stderr)
            return 2
        try:
            lines = capture_raw(args.port, args.duration_s)
            _write_raw_record(args.output, lines)
        except (OSError, RuntimeError) as error:
            print(f"capture failed: {error}", file=sys.stderr)
            return 2
        print(f"raw record created: {args.output} ({len(lines)} lines)")
        if args.analyze:
            run = analyze_file(args.output)
            _print_metrics(args.output.name, run)
        return 0

    if args.command == "compare":
        try:
            return compare_runs(args.baseline, args.sync)
        except OSError as error:
            print(f"comparison failed: {error}", file=sys.stderr)
            return 2

    return 2


if __name__ == "__main__":
    raise SystemExit(main())
