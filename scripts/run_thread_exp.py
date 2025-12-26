import subprocess
import os
import argparse
import sys

def run_thread_test(exec_path, num_threads, output_dir):
    command = ["perf", "stat", "-e", "cache-misses,L1-dcache-load-misses,LLC-load-misses", exec_path, str(num_threads)]

    result = subprocess.run(
        command,
        capture_output=True,
        text=True
    )

    exp_res_path = os.path.join(output_dir, f"thread_exp_{num_threads}.txt")
    with open(exp_res_path, "w") as f:
        f.write(result.stderr)
    
    stats_path = os.path.join(output_dir, f"thread_exp_{num_threads}_stats.txt")
    with open(stats_path, "w") as f:
        f.write(result.stdout)

    print(f"Run for {num_threads} threads completed. Results in {exp_res_path} and {stats_path}")

import re

def parse_perf_output(filepath):
    metrics = {}
    with open(filepath, 'r') as f:
        content = f.read()
        
        # Time elapsed
        time_match = re.search(r'(\d+\.\d+)\s+seconds time elapsed', content)
        if time_match:
            metrics['time'] = float(time_match.group(1))
            
        # Cache misses (sum of atom and core if present)
        cache_misses = 0
        for match in re.finditer(r'(\d[\d,]*)\s+.*cache-misses/', content):
            cache_misses += int(match.group(1).replace(',', ''))
        metrics['cache_misses'] = cache_misses

    return metrics

def parse_stats_output(filepath):
    total_tasks = 0
    total_attempts = 0
    total_successes = 0
    
    with open(filepath, 'r') as f:
        for line in f:
            if "Tasks Executed" in line:
                # Worker 0: Tasks Executed = 32626652, Steal Attempts = 957, Steal Successes = 380
                tasks = re.search(r'Tasks Executed = (\d+)', line)
                attempts = re.search(r'Steal Attempts = (\d+)', line)
                successes = re.search(r'Steal Successes = (\d+)', line)
                
                if tasks: total_tasks += int(tasks.group(1))
                if attempts: total_attempts += int(attempts.group(1))
                if successes: total_successes += int(successes.group(1))
                
    return {
        'tasks': total_tasks,
        'attempts': total_attempts,
        'successes': total_successes
    }

def get_system_info():
    try:
        cpu_info = subprocess.check_output("lscpu | grep 'Model name'", shell=True).decode().strip()
        return cpu_info
    except:
        return "Unknown CPU"

def generate_report(thread_counts, output_dir, baseline_dir=None):
    report = "# Performance Report\n\n"
    report += f"**System**: {get_system_info()}\n\n"
    
    headers = ["Threads", "Time (s)", "Cache Misses", "Tasks Executed", "Steal Attempts", "Steal Successes", "Efficiency"]
    if baseline_dir:
        headers.insert(2, "Baseline (s)")
        headers.insert(3, "Speedup")
        
    report += "| " + " | ".join(headers) + " |\n"
    report += "|" + "---|" * len(headers) + "\n"
    
    for n in thread_counts:
        perf_path = os.path.join(output_dir, f"thread_exp_{n}.txt")
        stats_path = os.path.join(output_dir, f"thread_exp_{n}_stats.txt")
        
        if not os.path.exists(perf_path) or not os.path.exists(stats_path):
            continue
            
        perf = parse_perf_output(perf_path)
        stats = parse_stats_output(stats_path)
        
        efficiency = (stats['successes'] / stats['attempts'] * 100) if stats['attempts'] > 0 else 0
        
        row = [
            str(n),
            f"{perf.get('time', 'N/A')}",
            f"{perf.get('cache_misses', 'N/A'):,}",
            f"{stats['tasks']:,}",
            f"{stats['attempts']:,}",
            f"{stats['successes']:,}",
            f"{efficiency:.2f}%"
        ]
        
        if baseline_dir:
            base_perf_path = os.path.join(baseline_dir, f"thread_exp_{n}.txt")
            base_time = "N/A"
            speedup = "N/A"
            
            if os.path.exists(base_perf_path):
                base_perf = parse_perf_output(base_perf_path)
                if 'time' in base_perf:
                    base_time = f"{base_perf['time']}"
                    if 'time' in perf and perf['time'] > 0:
                        speedup_val = base_perf['time'] / perf['time']
                        speedup = f"{speedup_val:.2f}x"
            
            row.insert(2, base_time)
            row.insert(3, speedup)
            
        report += "| " + " | ".join(row) + " |\n"
        
    report_path = os.path.join(output_dir, "performance_report.md")
    with open(report_path, "w") as f:
        f.write(report)
    print(f"Report generated: {report_path}")

import datetime
import shutil

def run_validation(exec_path, thread_counts):
    print("Running validation...")
    for n in thread_counts:
        command = [exec_path, str(n), "--validate"]
        result = subprocess.run(command, capture_output=True, text=True)
        if "Validation Passed" in result.stdout:
            print(f"Validation Passed for {n} threads.")
        else:
            print(f"WARNING: Validation FAILED for {n} threads!")
            print(result.stdout)

def generate_comparison_report(app_dir, thread_counts):
    report = "# Performance Comparison Report\n\n"
    report += "| Run | " + " | ".join([f"{t}T Time (s) | {t}T Eff (%)" for t in thread_counts]) + " |\n"
    report += "|---|" + "---|---|" * len(thread_counts) + "\n"

    runs = []
    for entry in os.scandir(app_dir):
        if entry.is_dir() and entry.name != "latest":
            runs.append(entry.name)
    runs.sort(reverse=True) # Newest first

    for run in runs:
        run_dir = os.path.join(app_dir, run)
        row = f"| {run} |"
        
        for n in thread_counts:
            perf_path = os.path.join(run_dir, f"thread_exp_{n}.txt")
            stats_path = os.path.join(run_dir, f"thread_exp_{n}_stats.txt")
            
            time_val = "N/A"
            eff_val = "N/A"

            if os.path.exists(perf_path):
                perf = parse_perf_output(perf_path)
                if 'time' in perf:
                    time_val = f"{perf['time']:.4f}"
            
            if os.path.exists(stats_path):
                stats = parse_stats_output(stats_path)
                if stats['attempts'] > 0:
                    eff = (stats['successes'] / stats['attempts'] * 100)
                    eff_val = f"{eff:.2f}"
            
            row += f" {time_val} | {eff_val} |"
        
        report += row + "\n"

    comp_path = os.path.join(app_dir, "comparison_report.md")
    with open(comp_path, "w") as f:
        f.write(report)
    print(f"Comparison report generated: {comp_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run thread scaling experiments.")
    parser.add_argument("--exec", help="Path to the runtime executable")
    parser.add_argument("--app", help="Name of the application (e.g., fib)")
    parser.add_argument("--baseline", help="Path to the baseline executable (e.g., cilk version)")
    args = parser.parse_args()

    if args.exec:
        exec_path = os.path.abspath(args.exec)
    elif args.app:
        exec_path = os.path.abspath(os.path.join("build", args.app))
    else:
        # Default to fib if nothing specified
        exec_path = os.path.abspath(os.path.join("build", "fib"))

    if not os.path.exists(exec_path):
        print(f"Error: Executable not found at {exec_path}")
        sys.exit(1)

    print(f"Using executable: {exec_path}")
    
    # Determine app name for output directory
    if args.app:
        app_name = args.app
    elif args.exec:
        app_name = os.path.basename(args.exec)
    else:
        app_name = "fib"
        
    # Timestamped directory
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    app_dir = os.path.join("temp", app_name)
    output_dir = os.path.join(app_dir, timestamp)
    os.makedirs(output_dir, exist_ok=True)
    print(f"Output directory: {output_dir}")

    # Update latest symlink
    latest_link = os.path.join(app_dir, "latest")
    if os.path.islink(latest_link):
        os.remove(latest_link)
    elif os.path.isdir(latest_link):
        shutil.rmtree(latest_link)
    
    os.symlink(timestamp, latest_link) # Relative symlink

    thread_counts = (8, 16, 32, 64)
    
    # 1. Run Validation
    run_validation(exec_path, thread_counts)
    
    # 2. Run Performance Experiments
    print("\nRunning performance experiments...")
    for num_threads in thread_counts:
        run_thread_test(exec_path, num_threads, output_dir)
        
    # 3. Run Baseline Experiments (if provided)
    baseline_dir = None
    if args.baseline:
        baseline_path = os.path.abspath(args.baseline)
        if os.path.exists(baseline_path):
            print(f"\nRunning baseline experiments using: {baseline_path}")
            baseline_dir = os.path.join(output_dir, "baseline")
            os.makedirs(baseline_dir, exist_ok=True)
            for num_threads in thread_counts:
                run_thread_test(baseline_path, num_threads, baseline_dir)
        else:
            print(f"Warning: Baseline executable not found at {baseline_path}")
    
    # 4. Generate Report (per run)
    generate_report(thread_counts, output_dir, baseline_dir)
    
    # 5. Generate Comparison Report
    generate_comparison_report(app_dir, thread_counts)
