#!/usr/bin/env python3
"""Estimate Hall pulses per revolution from reference-RPM measurements."""

from __future__ import annotations

import argparse
import csv
import math
import statistics
from dataclasses import dataclass
from pathlib import Path


REQUIRED_FIELDS = ("motor_id", "point_id", "reference_rpm", "hall_period_us")
OUTPUT_FIELDS = REQUIRED_FIELDS + (
    "pulse_frequency_hz", "ppr_estimate", "candidate_ppr", "calculated_rpm",
    "observed_error_percent",
)


@dataclass(frozen=True)
class Measurement:
    motor_id: str
    point_id: str
    reference_rpm: float
    hall_period_us: float


@dataclass(frozen=True)
class Result:
    measurement: Measurement
    pulse_frequency_hz: float
    ppr_estimate: float
    candidate_ppr: int
    calculated_rpm: float
    observed_error_percent: float


@dataclass(frozen=True)
class Summary:
    motor_id: str
    point_count: int
    candidate_ppr: int
    median_ppr_estimate: float
    maximum_observed_error_percent: float
    qualification: str


def _positive_finite(value: str, field: str, line_number: int) -> float:
    try:
        parsed = float(value)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"line {line_number}: field {field!r} must be numeric") from exc
    if not math.isfinite(parsed) or parsed <= 0.0:
        raise ValueError(f"line {line_number}: field {field!r} must be positive and finite")
    return parsed


def load_measurements(path: Path) -> list[Measurement]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames is None:
            raise ValueError("CSV is empty or has no header")
        missing = [field for field in REQUIRED_FIELDS if field not in reader.fieldnames]
        if missing:
            raise ValueError(f"missing required fields: {', '.join(missing)}")

        measurements: list[Measurement] = []
        for line_number, row in enumerate(reader, start=2):
            motor_id = (row.get("motor_id") or "").strip()
            point_id = (row.get("point_id") or "").strip()
            if not motor_id:
                raise ValueError(f"line {line_number}: field 'motor_id' must not be empty")
            if not point_id:
                raise ValueError(f"line {line_number}: field 'point_id' must not be empty")
            measurements.append(Measurement(
                motor_id=motor_id,
                point_id=point_id,
                reference_rpm=_positive_finite(row.get("reference_rpm", ""), "reference_rpm", line_number),
                hall_period_us=_positive_finite(row.get("hall_period_us", ""), "hall_period_us", line_number),
            ))
    if not measurements:
        raise ValueError("CSV contains no data rows")
    return measurements


def analyze(measurements: list[Measurement], target_error_percent: float,
            reference_accuracy_percent: float | None) -> tuple[list[Result], list[Summary]]:
    if not math.isfinite(target_error_percent) or target_error_percent <= 0.0:
        raise ValueError("target error must be positive and finite")
    if reference_accuracy_percent is not None and (
        not math.isfinite(reference_accuracy_percent) or reference_accuracy_percent < 0.0
    ):
        raise ValueError("reference accuracy must be non-negative and finite")

    grouped: dict[str, list[tuple[Measurement, float, float]]] = {}
    for measurement in measurements:
        frequency = 1_000_000.0 / measurement.hall_period_us
        estimate = 60.0 * frequency / measurement.reference_rpm
        grouped.setdefault(measurement.motor_id, []).append((measurement, frequency, estimate))

    results: list[Result] = []
    summaries: list[Summary] = []
    for motor_id in sorted(grouped):
        group = grouped[motor_id]
        median_estimate = statistics.median(item[2] for item in group)
        candidate = max(1, math.floor(median_estimate + 0.5))
        motor_results: list[Result] = []
        for measurement, frequency, estimate in group:
            calculated_rpm = 60.0 * frequency / candidate
            error = abs(calculated_rpm - measurement.reference_rpm) / measurement.reference_rpm * 100.0
            motor_results.append(Result(measurement, frequency, estimate, candidate,
                                        calculated_rpm, error))
        results.extend(motor_results)
        maximum_error = max(result.observed_error_percent for result in motor_results)
        unique_points = len({result.measurement.point_id for result in motor_results})
        if unique_points < 2:
            qualification = "INSUFFICIENT_POINTS"
        elif reference_accuracy_percent is None:
            qualification = "UNVERIFIED_REFERENCE_ACCURACY"
        elif maximum_error + reference_accuracy_percent <= target_error_percent:
            qualification = "PASS_CONSERVATIVE_BOUND"
        elif maximum_error > target_error_percent:
            qualification = "FAIL_OBSERVED_ERROR"
        else:
            qualification = "INCONCLUSIVE_REFERENCE_ACCURACY"
        summaries.append(Summary(motor_id, unique_points, candidate, median_estimate,
                                 maximum_error, qualification))
    return results, summaries


def write_results(path: Path, results: list[Result]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=OUTPUT_FIELDS)
        writer.writeheader()
        for result in results:
            writer.writerow({
                "motor_id": result.measurement.motor_id,
                "point_id": result.measurement.point_id,
                "reference_rpm": f"{result.measurement.reference_rpm:.6f}",
                "hall_period_us": f"{result.measurement.hall_period_us:.6f}",
                "pulse_frequency_hz": f"{result.pulse_frequency_hz:.6f}",
                "ppr_estimate": f"{result.ppr_estimate:.6f}",
                "candidate_ppr": result.candidate_ppr,
                "calculated_rpm": f"{result.calculated_rpm:.6f}",
                "observed_error_percent": f"{result.observed_error_percent:.6f}",
            })


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", type=Path, help="input calibration CSV")
    parser.add_argument("--output", type=Path, required=True, help="new processed CSV path")
    parser.add_argument("--target-error-percent", type=float, default=3.0)
    parser.add_argument("--reference-accuracy-percent", type=float)
    parser.add_argument("--reference-source",
                        help="traceable manual, certificate, or specification for reference accuracy")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.reference_source is not None and not args.reference_source.strip():
        raise SystemExit("--reference-source must not be empty")
    if (args.reference_accuracy_percent is None) != (args.reference_source is None):
        raise SystemExit("--reference-accuracy-percent and --reference-source must be provided together")
    measurements = load_measurements(args.csv_path)
    results, summaries = analyze(measurements, args.target_error_percent,
                                 args.reference_accuracy_percent)
    write_results(args.output, results)
    print(f"wrote {len(results)} rows to {args.output}")
    if args.reference_source is None:
        print("reference accuracy: UNVERIFIED (no traceable source supplied)")
    else:
        print(f"reference accuracy: ±{args.reference_accuracy_percent:.6g}% "
              f"(source: {args.reference_source})")
    for summary in summaries:
        print(f"motor={summary.motor_id} points={summary.point_count} "
              f"candidate_ppr={summary.candidate_ppr} "
              f"median_estimate={summary.median_ppr_estimate:.6f} "
              f"max_observed_error={summary.maximum_observed_error_percent:.6f}% "
              f"qualification={summary.qualification}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
