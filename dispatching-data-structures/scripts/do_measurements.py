#!/usr/bin/env python3

import json
import os
import shlex
import subprocess
import sys
import argparse

PROJECT_ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


def critical(message):
    print(message, file=sys.stderr)
    exit(1)


def ensure_file_exists(filepath):
    if not os.path.isfile(filepath):
        critical(f"File {filepath} does not exist.")


def ensure_file_executable(filepath):
    ensure_file_exists(filepath)
    if not os.access(filepath, os.X_OK):
        critical(f"File {filepath} is not exectuable.")


def run_benchmark(iteration_count: int, packet_file: str, analyzer_mapping_file: str, executable: str):
    packet_file = packet_file.format(0)
    ensure_file_exists(packet_file)
    ensure_file_exists(analyzer_mapping_file)
    ensure_file_exists(executable)

    if not executable.startswith("/"):
        executable = "./" + executable

    measurements = [dict(run=i + 1) for i in range(iteration_count)]

    cmd = f"{executable} {packet_file} {analyzer_mapping_file} {iteration_count} --benchmark_format=json"
    p = subprocess.Popen(
        shlex.split(f"taskset 0x1 {cmd}"),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    out, err = p.communicate()

    if p.returncode != 0:
        critical(f"Benchmark application run '{packet_file} {analyzer_mapping_file}' failed:\n{err.decode('utf-8')}")
        return

    try:
        data = json.loads(out.decode("utf-8"))
    except json.JSONDecodeError as e:
        critical(f"Benchmark application run '{packet_file} {analyzer_mapping_file}' yielded invalid JSON data: {e}")
        return

    try:
        for run in data["benchmarks"]:
            if run["run_type"] != "iteration":
                continue

            data_structure_name = run["run_name"].split("/")[0].lower()
            run_dict = measurements[run["repetition_index"]]
            if data_structure_name not in run_dict:
                run_dict[data_structure_name] = dict()

            run_dict[data_structure_name]["dispatch_time_real"] = run["real_time"]
            run_dict[data_structure_name]["dispatch_time_cpu"] = run["cpu_time"]
    except KeyError:
        critical(
            f"Benchmark application run '{packet_file} {analyzer_mapping_file}'"
            f" yielded JSON data with incomplete information."
        )

    return dict(
        mapping_name=os.path.basename(analyzer_mapping_file),
        trace_name=os.path.basename(packet_file).split("_")[0],
        measurements=measurements
    )


def run_benchmark_ex(iteration_count: int, packet_file: str, analyzer_mapping_file: str, executable: str, startup: bool):
    ensure_file_exists(analyzer_mapping_file)
    ensure_file_exists(executable)

    if not executable.startswith("/"):
        executable = "./" + executable

    measurements = []

    for iteration in range(iteration_count):
        current_packet_file = packet_file.format(iteration)
        ensure_file_exists(current_packet_file)

        print(
            f"  Run for '{os.path.basename(current_packet_file)}' trace",
            file=sys.stderr
        )

        cmd = f"{executable} {current_packet_file} {analyzer_mapping_file} 1{' startup' if startup else ''} --benchmark_format=json"
        p = subprocess.Popen(
            shlex.split(f"taskset 0x1 {cmd}"),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        out, err = p.communicate()

        if p.returncode != 0:
            critical(f"Benchmark application run '{current_packet_file} {analyzer_mapping_file}' failed:\n{err.decode('utf-8')}")
            return

        try:
            data = json.loads(out.decode("utf-8"))
        except json.JSONDecodeError as e:
            critical(f"Benchmark application run '{current_packet_file} {analyzer_mapping_file}' yielded invalid JSON data: {e}")
            return

        try:
            name = "startup" if startup else "dispatch"
            run_dict = dict(run=iteration + 1)
            for run in data["benchmarks"]:
                if run["run_type"] != "iteration":
                    continue

                data_structure_name = run["run_name"].split("/")[0].lower()
                if data_structure_name not in run_dict:
                    run_dict[data_structure_name] = dict()

                run_dict[data_structure_name][f"{name}_time_real"] = run["real_time"]
                run_dict[data_structure_name][f"{name}_time_cpu"] = run["cpu_time"]

            measurements.append(run_dict)
        except KeyError:
            critical(
                f"Benchmark application run '{current_packet_file} {analyzer_mapping_file}'"
                f" yielded JSON data with incomplete information."
            )

    return dict(
        mapping_name=os.path.basename(analyzer_mapping_file),
        trace_name=os.path.basename(packet_file).split("_")[0],
        measurements=measurements
    )


def run_cache_analysis(iteration_count: int, packet_file: str, analyzer_mapping_file: str, executable: str):
    ensure_file_exists(packet_file)
    ensure_file_exists(analyzer_mapping_file)
    ensure_file_exists(executable)

    if not executable.startswith("/"):
        executable = "./" + executable

    measurements = []

    for iteration in range(iteration_count):
        cmd = f"{executable} {packet_file} {analyzer_mapping_file}"
        p = subprocess.Popen(
            shlex.split(f"taskset 0x1 {cmd}"),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        out, err = p.communicate()

        if p.returncode != 0:
            critical(
                f"Cache analyzer application run '{packet_file} {analyzer_mapping_file}' failed:\n"
                f"{err.decode('utf-8')}"
            )
            return

        lines = out.decode("utf-8").strip().split("\n")
        if len(lines) < 2:
            critical(
                f"Cache analyzer application run '{packet_file} {analyzer_mapping_file}'"
                f" yielded less than 2 rows."
            )

        names = lines[0].split(",")
        run_dict = dict(run=iteration + 1)
        for line in lines[1:]:
            data = line.split(",")
            if len(data) < 4:
                critical(
                    f"Cache analyzer application run '{packet_file} {analyzer_mapping_file}'"
                    f" yielded a row with not enough columns."
                )
            elif len(data) != len(names):
                critical(
                    f"Cache analyzer application run '{packet_file} {analyzer_mapping_file}'"
                    f" yielded a row with a different column count than the header row."
                )

            if data[0] not in run_dict:
                run_dict[data[0]] = dict()

            for idx, value in enumerate(data[1:]):
                run_dict[data[0]][names[idx + 1]] = int(value)

        measurements.append(run_dict)

    return dict(
        mapping_name=os.path.basename(analyzer_mapping_file),
        trace_name=os.path.basename(packet_file).split("_")[0],
        measurements=measurements
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Tool that allows doing measurements about dispatching data structures."
                    " Results are printed as JSON to stdout."
    )
    parser.add_argument(
        "executable",
        help="Path to the executable to run with."
             " Must be either the dispatching time or the cache miss analysis executable."
    )
    parser.add_argument(
        "--startup", "-s", action="store_true",
        help="Run startup time instead of dispatching time benchmark."
    )
    args = parser.parse_args()


    iterations = 10
    if os.path.basename(args.executable) == "benchmark":
        # Generate the data necessary for the paper plots.
        if args.startup:
            benchmark_runs = [
                ["input/traces/cic-ids17-mon", "input/analyzers/fragmented"],
            ]
        else:
            benchmark_runs = [
                ["input/traces/rand_{}", "input/analyzers/zeek"],
                ["input/traces/rand_{}", "input/analyzers/fragmented"],
                ["input/traces/cic-ids17-mon", "input/analyzers/zeek"],
                ["input/traces/cic-ids17-mon", "input/analyzers/fragmented"],
            ]

        benchmark_results = []
        for benchmark_run in benchmark_runs:
            print(
                f"Running {'startup' if args.startup else 'dispatching'} time benchmark"
                f" with '{os.path.basename(benchmark_run[0])}' trace"
                f" and '{os.path.basename(benchmark_run[1])}' analyzer mapping...",
                file=sys.stderr
            )
            benchmark_results.append(
                run_benchmark_ex(
                    iterations,
                    os.path.join(PROJECT_ROOT, benchmark_run[0]),
                    os.path.join(PROJECT_ROOT, benchmark_run[1]),
                    args.executable,
                    args.startup
                )
            )

        print(json.dumps(benchmark_results))
    elif os.path.basename(args.executable) == "cache_analyzer":
        cache_runs = [
            ["input/traces/cic-ids17-mon", "input/analyzers/zeek"],
            ["input/traces/cic-ids17-mon", "input/analyzers/fragmented"],
        ]

        cache_results = []
        for cache_run in cache_runs:
            print(
                f"Running cache miss analysis with '{os.path.basename(cache_run[0])}' trace"
                f" and '{os.path.basename(cache_run[1])}' analyzer mapping...",
                file=sys.stderr
            )
            cache_results.append(
                run_cache_analysis(
                    iterations,
                    os.path.join(PROJECT_ROOT, cache_run[0]),
                    os.path.join(PROJECT_ROOT, cache_run[1]),
                    args.executable
                )
            )

        print(json.dumps(cache_results))
    else:
        critical("Invalid executable provided.")
