#!/usr/bin/env python3
"""Validate and plot dual-motor RPM telemetry CSV logs."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Iterable


REQUIRED_FIELDS = (
    "timestamp_ms",
    "base_pwm_us",
    "rpm1",
    "rpm2",
    "error_rpm",
    "error_percent",
    "correction_us",
    "pwm1_us",
    "pwm2_us",
    "system_state",
    "fault_flags",
)

NUMERIC_FIELDS = REQUIRED_FIELDS[:9]


def _non_comment_lines(lines: Iterable[str]) -> Iterable[str]:
    for line in lines:
        if line.strip() and not line.lstrip().startswith("#"):
            yield line


def load_csv(path: Path) -> list[dict[str, object]]:
    """Load a telemetry CSV after validating schema and numeric fields."""
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(_non_comment_lines(stream))
        if reader.fieldnames is None:
            raise ValueError("CSV is empty or has no header")

        missing = [field for field in REQUIRED_FIELDS if field not in reader.fieldnames]
        if missing:
            raise ValueError(f"missing required fields: {', '.join(missing)}")

        rows: list[dict[str, object]] = []
        for line_number, source in enumerate(reader, start=2):
            row: dict[str, object] = dict(source)
            try:
                for field in NUMERIC_FIELDS:
                    row[field] = float(source[field])
            except (TypeError, ValueError) as exc:
                raise ValueError(
                    f"line {line_number}: field {field!r} must be numeric"
                ) from exc
            rows.append(row)

    if not rows:
        raise ValueError("CSV contains no data rows")
    return rows


def plot_rows(rows: list[dict[str, object]], output: Path | None, show: bool) -> None:
    """Create the standard five-panel engineering plot."""
    import matplotlib.pyplot as plt

    time_s = [float(row["timestamp_ms"]) / 1000.0 for row in rows]
    fig, axes = plt.subplots(5, 1, figsize=(11, 13), sharex=True)

    axes[0].plot(time_s, [row["rpm1"] for row in rows], label="RPM1")
    axes[0].plot(time_s, [row["rpm2"] for row in rows], label="RPM2")
    axes[0].set_ylabel("RPM")
    axes[0].legend()

    axes[1].plot(time_s, [row["error_rpm"] for row in rows], color="tab:red")
    axes[1].axhline(0.0, color="black", linewidth=0.7)
    axes[1].set_ylabel("Error (RPM)")

    axes[2].plot(time_s, [row["error_percent"] for row in rows], color="tab:orange")
    axes[2].set_ylabel("Error (%)")

    axes[3].plot(time_s, [row["correction_us"] for row in rows], color="tab:green")
    axes[3].axhline(0.0, color="black", linewidth=0.7)
    axes[3].set_ylabel("Correction (us)")

    axes[4].plot(time_s, [row["pwm1_us"] for row in rows], label="PWM1")
    axes[4].plot(time_s, [row["pwm2_us"] for row in rows], label="PWM2")
    axes[4].set_ylabel("PWM (us)")
    axes[4].set_xlabel("Time (s)")
    axes[4].legend()

    for axis in axes:
        axis.grid(True, alpha=0.3)

    fig.suptitle("Dual-motor RPM telemetry")
    fig.tight_layout()

    if output is not None:
        output.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(output, dpi=160)
    if show:
        plt.show()
    plt.close(fig)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", type=Path, help="input telemetry CSV")
    parser.add_argument("--output", type=Path, help="write PNG/PDF/SVG instead of only validating")
    parser.add_argument("--show", action="store_true", help="open an interactive plot window")
    parser.add_argument("--validate-only", action="store_true", help="validate without importing matplotlib")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    rows = load_csv(args.csv_path)
    print(f"validated {len(rows)} rows from {args.csv_path}")
    if not args.validate_only:
        if args.output is None and not args.show:
            raise SystemExit("choose --output, --show, or --validate-only")
        plot_rows(rows, args.output, args.show)
        if args.output is not None:
            print(f"wrote plot to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
