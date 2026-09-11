#!/usr/bin/env python3
"""Capture and validate dual PX4 PWM-input UART telemetry."""

from __future__ import annotations

import argparse
import sys
import time
from dataclasses import dataclass
from pathlib import Path


BAUD_RATE = 115200
EXPECTED_SOURCE = "rpm_sync_pwm_input"
EXPECTED_VERSION = "v1"
EXPECTED_KEYS = (
    "t_ms",
    "ch1_seen",
    "ch1_period_us",
    "ch1_raw_us",
    "ch1_us",
    "ch1_age_ms",
    "ch1_status",
    "ch2_seen",
    "ch2_period_us",
    "ch2_raw_us",
    "ch2_us",
    "ch2_age_ms",
    "ch2_status",
)
STATUSES = ("WAITING", "VALID", "TIMED_OUT", "OUT_OF_RANGE", "INVALID_CONFIG")
UINT32_MAX = (1 << 32) - 1


@dataclass(frozen=True)
class PwmInputSample:
    timestamp_ms: int
    seen: tuple[bool, bool]
    period_us: tuple[int, int]
    raw_us: tuple[int, int]
    pulse_width_us: tuple[int, int]
    age_ms: tuple[int, int]
    status: tuple[str, str]


def _parse_uint32(value: str, name: str) -> int:
    if not value.isdecimal():
        raise ValueError(f"{name} must be an unsigned decimal integer")
    parsed = int(value)
    if parsed > UINT32_MAX:
        raise ValueError(f"{name} exceeds uint32")
    return parsed


def _validate_channel(sample: PwmInputSample, index: int) -> None:
    seen = sample.seen[index]
    period_us = sample.period_us[index]
    raw_us = sample.raw_us[index]
    pulse_width_us = sample.pulse_width_us[index]
    status = sample.status[index]
    prefix = f"ch{index + 1}"

    if status == "VALID":
        if not seen or period_us == 0 or raw_us == 0 or pulse_width_us != raw_us:
            raise ValueError(f"{prefix} VALID fields are inconsistent")
        return

    if pulse_width_us != 0:
        raise ValueError(f"{prefix} non-VALID status must have zero effective width")
    if status == "WAITING" and (seen or period_us != 0 or raw_us != 0):
        raise ValueError(f"{prefix} WAITING fields are inconsistent")
    if status in ("TIMED_OUT", "OUT_OF_RANGE") and (
        not seen or period_us == 0
    ):
        raise ValueError(f"{prefix} {status} must retain the captured sample")


def parse_pwm_input_line(line: str) -> PwmInputSample:
    """Parse one exact PWM-input telemetry line or raise ValueError."""
    parts = line.strip().split(",")
    if len(parts) < 2 or parts[:2] != [EXPECTED_SOURCE, EXPECTED_VERSION]:
        raise ValueError("unexpected PWM-input source or version")
    if len(parts) != len(EXPECTED_KEYS) + 2:
        raise ValueError("PWM-input line contains the wrong number of fields")

    fields: dict[str, str] = {}
    for part in parts[2:]:
        if "=" not in part:
            raise ValueError("PWM-input fields must use key=value")
        key, value = part.split("=", 1)
        if key in fields:
            raise ValueError(f"duplicate PWM-input field: {key}")
        fields[key] = value
    if tuple(fields) != EXPECTED_KEYS:
        raise ValueError("unexpected PWM-input field order or names")

    numeric_keys = tuple(key for key in EXPECTED_KEYS if not key.endswith("_status"))
    values = {key: _parse_uint32(fields[key], key) for key in numeric_keys}
    for key in ("ch1_seen", "ch2_seen"):
        if values[key] not in (0, 1):
            raise ValueError(f"{key} must be 0 or 1")
    statuses = (fields["ch1_status"], fields["ch2_status"])
    if any(status not in STATUSES for status in statuses):
        raise ValueError("unknown PWM-input status")

    sample = PwmInputSample(
        timestamp_ms=values["t_ms"],
        seen=(bool(values["ch1_seen"]), bool(values["ch2_seen"])),
        period_us=(values["ch1_period_us"], values["ch2_period_us"]),
        raw_us=(values["ch1_raw_us"], values["ch2_raw_us"]),
        pulse_width_us=(values["ch1_us"], values["ch2_us"]),
        age_ms=(values["ch1_age_ms"], values["ch2_age_ms"]),
        status=statuses,
    )
    _validate_channel(sample, 0)
    _validate_channel(sample, 1)
    return sample


def write_raw_record(path: Path, lines: list[str]) -> None:
    """Create a new raw record without overwriting an existing file."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8", newline="") as output:
        for line in lines:
            output.write(line.rstrip("\r\n") + "\n")


def capture_samples(
    port: str,
    duration_s: float,
    required_samples: int,
    require_both_valid: bool,
) -> tuple[list[str], list[PwmInputSample]]:
    try:
        import serial
    except ImportError as error:
        raise RuntimeError(
            "pyserial is missing; install tools/requirements.txt in the project venv"
        ) from error

    lines: list[str] = []
    samples: list[PwmInputSample] = []
    deadline = time.monotonic() + duration_s
    with serial.Serial(port=port, baudrate=BAUD_RATE, timeout=min(1.0, duration_s)) as connection:
        while time.monotonic() < deadline:
            raw_line = connection.readline()
            if not raw_line:
                continue
            line = raw_line.decode("utf-8", errors="replace").rstrip("\r\n")
            lines.append(line)
            try:
                samples.append(parse_pwm_input_line(line))
            except ValueError:
                continue
            if require_both_valid:
                accepted = sum(
                    sample.status == ("VALID", "VALID") for sample in samples
                )
            else:
                accepted = len(samples)
            if accepted >= required_samples:
                break
    return lines, samples


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="serial device selected locally")
    parser.add_argument("--duration-s", type=float, default=15.0)
    parser.add_argument("--required-samples", type=int, default=5)
    parser.add_argument("--require-both-valid", action="store_true")
    parser.add_argument("--output", type=Path, help="new raw record; never overwritten")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.duration_s <= 0 or args.required_samples <= 0:
        print("duration and required sample count must be positive", file=sys.stderr)
        return 2
    try:
        lines, samples = capture_samples(
            args.port,
            args.duration_s,
            args.required_samples,
            args.require_both_valid,
        )
        if args.output is not None:
            write_raw_record(args.output, lines)
    except (OSError, RuntimeError) as error:
        print(f"capture failed: {error}", file=sys.stderr)
        return 2

    if len(samples) < args.required_samples:
        print(f"capture failed: received {len(samples)}/{args.required_samples} valid lines", file=sys.stderr)
        return 1
    both_valid = [sample for sample in samples if sample.status == ("VALID", "VALID")]
    if args.require_both_valid and len(both_valid) < args.required_samples:
        print(f"capture failed: only {len(both_valid)}/{args.required_samples} samples have both channels VALID", file=sys.stderr)
        return 1

    selected = both_valid if args.require_both_valid else samples
    print(f"capture ok: {len(samples)} samples ({len(both_valid)} both VALID)")
    if both_valid:
        differences = [abs(s.pulse_width_us[0] - s.pulse_width_us[1]) for s in both_valid]
        print(f"both-valid maximum channel difference: {max(differences)} us")
    if args.output is not None:
        print(f"raw record created: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
