#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import os
from dataclasses import dataclass
from pathlib import Path
import sys
from typing import List, Optional


def _import_matplotlib_pyplot():
    try:
        import matplotlib.pyplot as plt  # type: ignore

        return plt
    except ModuleNotFoundError:
        repo_root = Path(__file__).resolve().parents[1]
        venv_python = repo_root / "venvLocal" / "bin" / "python"
        if venv_python.exists() and sys.prefix == sys.base_prefix:
            os.execv(str(venv_python), [str(venv_python), *sys.argv])
        raise


plt = _import_matplotlib_pyplot()


@dataclass
class TimingSample:
    record_type: str
    iteration: int
    iteration_time_ms: float
    average_time_ms: float
    total_time_ms: float


def parse_csv(path: Path) -> tuple[List[TimingSample], Optional[TimingSample]]:
    samples: List[TimingSample] = []
    summary: Optional[TimingSample] = None

    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.reader(handle)
        for row in reader:
            if not row:
                continue

            if len(row) < 5:
                continue

            if row[0].strip().lower() == "record_type":
                continue

            try:
                sample = TimingSample(
                    record_type=row[0].strip(),
                    iteration=int(float(row[1])),
                    iteration_time_ms=float(row[2]),
                    average_time_ms=float(row[3]),
                    total_time_ms=float(row[4]),
                )
            except ValueError:
                continue

            if sample.record_type == "summary":
                summary = sample
            else:
                samples.append(sample)

    return samples, summary


def plot_samples(samples: List[TimingSample], summary: Optional[TimingSample], output_dir: Path, stem: str) -> None:
    if not samples:
        raise RuntimeError("No iteration samples found in CSV.")

    output_dir.mkdir(parents=True, exist_ok=True)

    iterations = [sample.iteration for sample in samples]
    iteration_times = [sample.iteration_time_ms for sample in samples]
    average_times = [sample.average_time_ms for sample in samples]
    total_times = [sample.total_time_ms for sample in samples]

    fig, axes = plt.subplots(2, 1, figsize=(12, 11), sharex=True)
    fig.suptitle("Radiosity Timing", fontsize=16, fontweight="bold")

    axes[0].plot(iterations, iteration_times, color="#2a6fdb", linewidth=1.8)
    axes[0].set_ylabel("Iteration time [ms]")
    axes[0].grid(True, alpha=0.25)

    axes[1].plot(iterations, average_times, color="#0f9d58", linewidth=1.8)
    axes[1].set_ylabel("Average time [ms]")
    axes[1].grid(True, alpha=0.25)

    if summary is not None:
        summary_text = (
            f"Coverage after {summary.iteration} iterations\n"
            f"Average frame time: {summary.average_time_ms:.3f} ms\n"
        )
        fig.text(0.02, 0.01, summary_text, fontsize=10, family="monospace")

    fig.tight_layout(rect=(0, 0.04, 1, 0.96))
    output_path = output_dir / f"{stem}_radiosity_timing.png"
    fig.savefig(output_path, dpi=180)
    plt.close(fig)

    fig2, ax2 = plt.subplots(figsize=(12, 5))
    ax2.plot(iterations, iteration_times, label="Iteration time", color="#2a6fdb", linewidth=1.8)
    ax2.plot(iterations, average_times, label="Running average", color="#0f9d58", linewidth=1.8)
    ax2.set_title("Radiosity Iteration Time vs Running Average")
    ax2.set_xlabel("Radiosity iteration")
    ax2.set_ylabel("Time [ms]")
    ax2.grid(True, alpha=0.25)
    ax2.legend()
    fig2.tight_layout()
    output_path2 = output_dir / f"{stem}_radiosity_iteration_vs_average.png"
    fig2.savefig(output_path2, dpi=180)
    plt.close(fig2)

    print(f"Saved plots to: {output_path.resolve()}")
    print(f"Saved plots to: {output_path2.resolve()}")
    if summary is not None:
        print(
            f"Coverage summary: iterations={summary.iteration}, "
            f"average_ms={summary.average_time_ms:.3f}, total_ms={summary.total_time_ms:.3f}"
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="Plot radiosity timing data from CSV.")
    parser.add_argument("input", nargs="?", default="radiosity_timing.csv", help="Input CSV file")
    parser.add_argument("-o", "--output-dir", default="radiosity_plots", help="Directory for generated plots")
    parser.add_argument("--stem", default="radiosity_timing", help="Base name for generated files")
    args = parser.parse_args()

    input_path = Path(args.input)
    output_dir = Path(args.output_dir)

    samples, summary = parse_csv(input_path)
    plot_samples(samples, summary, output_dir, args.stem)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())