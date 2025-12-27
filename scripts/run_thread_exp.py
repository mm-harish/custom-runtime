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
    comp_time = "N/A"
    
    stats_found = False
    with open(filepath, 'r') as f:
        for line in f:
            if "Computation Time:" in line:
                match = re.search(r'Computation Time: (\d+\.\d+) seconds', line)
                if match:
                    comp_time = float(match.group(1))
            if "Tasks Executed" in line:
                stats_found = True
                # Worker 0: Tasks Executed = 32626652, Steal Attempts = 957, Steal Successes = 380
                tasks = re.search(r'Tasks Executed = (\d+)', line)
                attempts = re.search(r'Steal Attempts = (\d+)', line)
                successes = re.search(r'Steal Successes = (\d+)', line)
                
                if tasks: total_tasks += int(tasks.group(1))
                if attempts: total_attempts += int(attempts.group(1))
                if successes: total_successes += int(successes.group(1))
                
    return {
        'tasks': total_tasks if stats_found else "N/A",
        'attempts': total_attempts if stats_found else "N/A",
        'successes': total_successes if stats_found else "N/A",
        'comp_time': comp_time
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
    
    headers = ["Threads", "Time (s)", "Comp Time (s)", "Cache Misses", "Tasks Executed", "Steal Attempts", "Steal Successes", "Efficiency"]
    if baseline_dir:
        headers.insert(3, "Base Comp (s)")
        headers.insert(4, "Comp Speedup")
        headers.insert(5, "Total Speedup")
        
    report += "| " + " | ".join(headers) + " |\n"
    report += "|" + "---|" * len(headers) + "\n"
    
    for n in thread_counts:
        perf_path = os.path.join(output_dir, f"thread_exp_{n}.txt")
        stats_path = os.path.join(output_dir, f"thread_exp_{n}_stats.txt")
        
        if not os.path.exists(perf_path) or not os.path.exists(stats_path):
            continue
            
        perf = parse_perf_output(perf_path)
        stats = parse_stats_output(stats_path)
        
        efficiency = 0
        if isinstance(stats['attempts'], (int, float)) and stats['attempts'] > 0:
            efficiency = (stats['successes'] / stats['attempts'] * 100)
        efficiency_str = f"{efficiency:.2f}%" if isinstance(stats['attempts'], (int, float)) else "N/A"
        
        row = [
            str(n),
            f"{perf.get('time', 'N/A')}",
            f"{stats.get('comp_time', 'N/A')}",
            f"{perf.get('cache_misses', 'N/A'):,}" if isinstance(perf.get('cache_misses'), (int, float)) else "N/A",
            f"{stats['tasks']:,}" if isinstance(stats['tasks'], (int, float)) else "N/A",
            f"{stats['attempts']:,}" if isinstance(stats['attempts'], (int, float)) else "N/A",
            f"{stats['successes']:,}" if isinstance(stats['successes'], (int, float)) else "N/A",
            efficiency_str
        ]
        
        if baseline_dir:
            base_perf_path = os.path.join(baseline_dir, f"thread_exp_{n}.txt")
            base_stats_path = os.path.join(baseline_dir, f"thread_exp_{n}_stats.txt")
            base_time = "N/A"
            base_comp = "N/A"
            comp_speedup = "N/A"
            total_speedup = "N/A"
            
            if os.path.exists(base_perf_path):
                base_perf = parse_perf_output(base_perf_path)
                if 'time' in base_perf:
                    base_time = f"{base_perf['time']}"
                    if 'time' in perf and perf['time'] > 0:
                        speedup_val = base_perf['time'] / perf['time']
                        total_speedup = f"{speedup_val:.2f}x"
            
            if os.path.exists(base_stats_path):
                base_stats = parse_stats_output(base_stats_path)
                if base_stats.get('comp_time') != "N/A":
                    base_comp = f"{base_stats['comp_time']}"
                    if stats.get('comp_time') != "N/A" and stats['comp_time'] > 0:
                        speedup_val = base_stats['comp_time'] / stats['comp_time']
                        comp_speedup = f"{speedup_val:.2f}x"
            
            row.insert(3, base_comp)
            row.insert(4, comp_speedup)
            row.insert(5, total_speedup)
            
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
        elif "Validation Disabled" in result.stdout:
            print(f"Validation Disabled (compiled out) for {n} threads.")
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
                if isinstance(stats['attempts'], (int, float)) and stats['attempts'] > 0:
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
