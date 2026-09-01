#!/usr/bin/env python3
"""Capture and validate the Issue #3 STM32 bring-up heartbeat."""

from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path


BAUD_RATE = 115200
EXPECTED_SOURCE = "rpm_sync_bringup"
EXPECTED_VERSION = "v1"
EXPECTED_FIELDS = {
    "board": "weact_g431_qfn48",
    "mode": "MONITOR_ONLY",
}


def parse_heartbeat(line: str) -> dict[str, str]:
    """Parse one exact bring-up heartbeat or raise ValueError."""
    parts = line.strip().split(",")
    if len(parts) != 4:
        raise ValueError("heartbeat must contain four comma-separated fields")
    if parts[0] != EXPECTED_SOURCE:
        raise ValueError("unexpected heartbeat source")
    if parts[1] != EXPECTED_VERSION:
        raise ValueError("unexpected heartbeat version")

    fields: dict[str, str] = {}
    for part in parts[2:]:
        if "=" not in part:
            raise ValueError("heartbeat metadata must use key=value")
        key, value = part.split("=", 1)
        if key in fields:
            raise ValueError(f"duplicate heartbeat field: {key}")
        fields[key] = value

    if fields != EXPECTED_FIELDS:
        raise ValueError("unexpected board or operating mode")
    return fields


def write_raw_record(path: Path, lines: list[str]) -> None:
    """Create a new immutable-by-convention raw record; never overwrite one."""
    with path.open("x", encoding="utf-8", newline="") as output:
        for line in lines:
            output.write(line.rstrip("\r\n") + "\n")


def capture_heartbeats(
    port: str, duration_s: float, required_heartbeats: int
) -> tuple[list[str], int]:
    """Read bounded serial lines and return all decoded lines plus valid count."""
    try:
        import serial
    except ImportError as error:
        raise RuntimeError(
            "pyserial is missing; install tools/requirements.txt in the project venv"
        ) from error

    received: list[str] = []
    valid_count = 0
    deadline = time.monotonic() + duration_s
    read_timeout_s = min(1.0, duration_s)

    with serial.Serial(
        port=port,
        baudrate=BAUD_RATE,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=read_timeout_s,
        xonxoff=False,
        rtscts=False,
        dsrdtr=False,
    ) as connection:
        while time.monotonic() < deadline and valid_count < required_heartbeats:
            raw_line = connection.readline()
            if not raw_line:
                continue
            line = raw_line.decode("utf-8", errors="replace").rstrip("\r\n")
            received.append(line)
            try:
                parse_heartbeat(line)
            except ValueError:
                continue
            valid_count += 1

    return received, valid_count


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="serial device selected locally")
    parser.add_argument("--duration-s", type=float, default=10.0)
    parser.add_argument("--required-heartbeats", type=int, default=3)
    parser.add_argument(
        "--output",
        type=Path,
        help="optional new raw text record; an existing file is never overwritten",
    )
    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.duration_s <= 0 or args.required_heartbeats <= 0:
        print("duration and required heartbeat count must be positive", file=sys.stderr)
        return 2

    try:
        lines, valid_count = capture_heartbeats(
            args.port, args.duration_s, args.required_heartbeats
        )
        if args.output is not None:
            write_raw_record(args.output, lines)
    except (OSError, RuntimeError) as error:
        print(f"capture failed: {error}", file=sys.stderr)
        return 2

    print(
        f"validated {valid_count}/{args.required_heartbeats} required heartbeats "
        f"from {len(lines)} received lines"
    )
    return 0 if valid_count >= args.required_heartbeats else 1


if __name__ == "__main__":
    raise SystemExit(main())
