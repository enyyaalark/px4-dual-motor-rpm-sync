#!/usr/bin/env python3
"""Capture and validate Issue #6 dual-Hall UART telemetry."""

from __future__ import annotations

import argparse
import sys
import time
from dataclasses import dataclass
from pathlib import Path


BAUD_RATE = 115200
EXPECTED_SOURCE = "rpm_sync_capture"
EXPECTED_VERSION = "v1"
EXPECTED_KEYS = (
    "t_ms",
    "ch1_valid",
    "ch1_period_us",
    "ch1_age_ms",
    "ch2_valid",
    "ch2_period_us",
    "ch2_age_ms",
)
UINT32_MAX = (1 << 32) - 1


@dataclass(frozen=True)
class CaptureSample:
    timestamp_ms: int
    valid: tuple[bool, bool]
    period_us: tuple[int, int]
    age_ms: tuple[int, int]


def _parse_uint32(value: str, name: str) -> int:
    if not value.isdecimal():
        raise ValueError(f"{name} must be an unsigned decimal integer")
    parsed = int(value)
    if parsed > UINT32_MAX:
        raise ValueError(f"{name} exceeds uint32")
    return parsed


def parse_capture_line(line: str) -> CaptureSample:
    """Parse one exact capture telemetry line or raise ValueError."""
    parts = line.strip().split(",")
    if len(parts) != 9:
        raise ValueError("capture line must contain nine comma-separated fields")
    if parts[0] != EXPECTED_SOURCE or parts[1] != EXPECTED_VERSION:
        raise ValueError("unexpected capture source or version")

    fields: dict[str, str] = {}
    for part in parts[2:]:
        if "=" not in part:
            raise ValueError("capture fields must use key=value")
        key, value = part.split("=", 1)
        if key in fields:
            raise ValueError(f"duplicate capture field: {key}")
        fields[key] = value
    if tuple(fields) != EXPECTED_KEYS:
        raise ValueError("unexpected capture field order or names")

    values = {key: _parse_uint32(fields[key], key) for key in EXPECTED_KEYS}
    for key in ("ch1_valid", "ch2_valid"):
        if values[key] not in (0, 1):
            raise ValueError(f"{key} must be 0 or 1")

    return CaptureSample(
        timestamp_ms=values["t_ms"],
        valid=(bool(values["ch1_valid"]), bool(values["ch2_valid"])),
        period_us=(values["ch1_period_us"], values["ch2_period_us"]),
        age_ms=(values["ch1_age_ms"], values["ch2_age_ms"]),
    )


def write_raw_record(path: Path, lines: list[str]) -> None:
    """Create a new raw record without overwriting an existing file."""
    with path.open("x", encoding="utf-8", newline="") as output:
        for line in lines:
            output.write(line.rstrip("\r\n") + "\n")


def capture_samples(
    port: str, duration_s: float, required_samples: int
) -> tuple[list[str], list[CaptureSample]]:
    try:
        import serial
    except ImportError as error:
        raise RuntimeError(
            "pyserial is missing; install tools/requirements.txt in the project venv"
        ) from error

    lines: list[str] = []
    samples: list[CaptureSample] = []
    deadline = time.monotonic() + duration_s
    with serial.Serial(
        port=port,
        baudrate=BAUD_RATE,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=min(1.0, duration_s),
        xonxoff=False,
        rtscts=False,
        dsrdtr=False,
    ) as connection:
        while time.monotonic() < deadline and len(samples) < required_samples:
            raw_line = connection.readline()
            if not raw_line:
                continue
            line = raw_line.decode("utf-8", errors="replace").rstrip("\r\n")
            lines.append(line)
            try:
                samples.append(parse_capture_line(line))
            except ValueError:
                continue
    return lines, samples


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="serial device selected locally")
    parser.add_argument("--duration-s", type=float, default=15.0)
    parser.add_argument("--required-samples", type=int, default=5)
    parser.add_argument("--require-both-valid", action="store_true")
    parser.add_argument(
        "--output", type=Path, help="new raw record; existing files are never overwritten"
    )
    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.duration_s <= 0 or args.required_samples <= 0:
        print("duration and required sample count must be positive", file=sys.stderr)
        return 2
    try:
        lines, samples = capture_samples(
            args.port, args.duration_s, args.required_samples
        )
        if args.output is not None:
            write_raw_record(args.output, lines)
    except (OSError, RuntimeError) as error:
        print(f"capture failed: {error}", file=sys.stderr)
        return 2

    both_valid = sum(all(sample.valid) for sample in samples)
    print(
        f"validated {len(samples)}/{args.required_samples} samples; "
        f"both channels valid in {both_valid}/{len(samples)}"
    )
    enough = len(samples) >= args.required_samples
    return 0 if enough and (not args.require_both_valid or both_valid > 0) else 1


if __name__ == "__main__":
    raise SystemExit(main())
