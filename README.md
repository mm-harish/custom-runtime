# Custom Work-Stealing Runtime

## Overview
This project implements a high-performance C++ runtime with a work-stealing scheduler, designed for efficient parallel execution of task-based applications. It currently includes a Fibonacci calculation application (`fib`) to demonstrate and benchmark the runtime.

## Prerequisites
- **C++ Compiler**: C++20 compatible (GCC or Clang recommended).
- **CMake**: Version 3.10 or higher.
- **Python 3**: For running experiment scripts.
- **perf**: Linux performance analysis tool (optional, for collecting hardware metrics).
- **Doxygen**: For generating API documentation (optional).

## Build Instructions
1. Create a build directory:
   ```bash
   mkdir build && cd build
   ```
2. Configure the project:
   ```bash
   cmake ..
   ```
3. Build the executables:
   ```bash
   make
   ```

## Usage

### Running the Application
The primary executable is `fib`. You can run it directly:

```bash
./build/fib <num_threads> [--validate]
```

- `<num_threads>`: Number of worker threads to use (default: 8).
- `--validate`: Optional flag to run a serial validation step after the parallel execution to verify correctness.

**Example:**
```bash
./build/fib 16 --validate
```

### Running Experiments
A Python script is provided to automate thread scaling experiments and generate a performance report.

```bash
python3 scripts/run_thread_exp.py --app fib
```

This script will:
1. **Validate**: Run the application with `--validate` for 8, 16, 32, and 64 threads to ensure correctness.
2. **Benchmark**: Run the application (without validation) to collect performance metrics (execution time, cache misses, internal runtime stats).
3. **Report**: Generate a `performance_report.md` file in the current directory with a summary table and system information.

## Documentation
To generate API documentation using Doxygen:

```bash
cd build
make doc_doxygen
```

The documentation will be generated in `build/docs/html/index.html`.

## Directory Structure
- **`apps/`**: Contains application-specific code (e.g., `fib/`).
- **`include/`**: Contains the core runtime header files (`wsqueue.h`, `task.h`, `runtime.h`, etc.).
- **`scripts/`**: Contains helper scripts for experiments (`run_thread_exp.py`).
- **`temp/`**: Directory where experiment results are stored.
    - **`<app_name>/<timestamp>/`**: Contains the results of a specific experiment run.
    - **`<app_name>/latest`**: Symlink to the most recent run directory.
    - **`<app_name>/comparison_report.md`**: Aggregated report comparing all runs for the application.

## Historical Comparison
The experiment script now supports historical comparison:
- **Timestamped Runs**: Each run is saved in a unique timestamped directory (e.g., `temp/fib/20231025_1000/`).
- **Comparison Report**: A `comparison_report.md` is generated in the application directory (`temp/fib/`), summarizing key metrics (Time, Efficiency) across all historical runs. This allows you to track performance changes over time.
