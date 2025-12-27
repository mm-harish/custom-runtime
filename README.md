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
   # Default: Validation OFF, Statistics ON
   cmake ..
   
   # Enable Validation, Disable Statistics
   cmake -DPERFORM_VALIDATION=ON ..
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
- `--validate`: Optional flag to run validation. Note that this only works if the project was built with `-DPERFORM_VALIDATION=ON`. If built with `OFF`, it will report that validation is disabled.

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

## Comparing with MIT Cilk (OpenCilk)

### Overview
This project supports comparison testing between the custom work-stealing runtime and MIT Cilk (OpenCilk). This allows you to benchmark and compare the performance characteristics of both parallel computing frameworks.

### Installing OpenCilk

#### Ubuntu 22.04 (Recommended)
1. Download the precompiled binary:
   ```bash
   wget https://github.com/OpenCilk/opencilk-project/releases/download/opencilk%2Fv2.1/opencilk-2.1.0-x86_64-linux-gnu-ubuntu-22.04.sh
   ```

2. Install to a directory (e.g., `/opt/opencilk`):
   ```bash
   sudo bash opencilk-2.1.0-x86_64-linux-gnu-ubuntu-22.04.sh --prefix=/opt/opencilk
   ```


   

3. Add OpenCilk to your PATH:
   ```bash
   export PATH=/opt/opencilk/bin:$PATH
   export LD_LIBRARY_PATH=/opt/opencilk/lib:$LD_LIBRARY_PATH
   ```
   
   Add these lines to your `~/.bashrc` to make them permanent.

#### Building from Source
For other systems or if you prefer to build from source, follow the instructions at: https://opencilk.org/doc/users-guide/install/

### Building with OpenCilk Support

Once OpenCilk is installed, rebuild the project:

```bash
cd build
cmake ..
make
```

If OpenCilk is detected, you'll see:
```
-- OpenCilk support detected - building fib_cilk
```

If not detected, you'll see a warning message with installation instructions.

### Running Comparison Tests

#### Quick Comparison (Recommended)
Use the provided convenience script:

```bash
./scripts/compare_runtimes.sh
```

This script will:
- Check if both executables are built
- Run validation tests for both runtimes
- Collect performance metrics (execution time, cache misses, etc.)
- Generate a comparison report

#### Manual Comparison
You can also run comparisons manually:

```bash
python3 scripts/run_thread_exp.py --app fib --baseline build/fib_cilk
```

### Interpreting Results

The comparison report (`temp/fib/latest/performance_report.md`) includes:

- **Time (s)**: Execution time for each runtime
- **Baseline (s)**: Execution time for the OpenCilk version
- **Speedup**: Ratio of baseline time to custom runtime time (>1.0 means custom runtime is faster)
- **Cache Misses**: Hardware cache miss statistics
- **Tasks Executed**: Total tasks executed (available only when `PERFORM_VALIDATION=OFF`)
- **Steal Attempts/Successes**: Work-stealing statistics (available only when `PERFORM_VALIDATION=OFF`)
- **Efficiency**: Work-stealing success rate (available only when `PERFORM_VALIDATION=OFF`)

#### What to Look For

1. **Execution Time**: Compare raw performance between runtimes
2. **Speedup**: Values close to 1.0x indicate similar performance
3. **Scalability**: How performance changes with thread count
4. **Cache Performance**: Lower cache misses generally indicate better memory locality
5. **Work-Stealing Efficiency**: Higher steal success rates indicate better load balancing

### Example Output

```markdown
| Threads | Time (s) | Baseline (s) | Speedup | Cache Misses | Tasks Executed | Steal Attempts | Steal Successes | Efficiency |
|---------|----------|--------------|---------|--------------|----------------|----------------|-----------------|------------|
| 8       | 0.234    | 0.241        | 1.03x   | 1,234,567    | 32,626,652     | 957            | 380             | 39.71%     |
| 16      | 0.128    | 0.135        | 1.05x   | 2,345,678    | 32,626,652     | 1,842          | 721             | 39.14%     |
```

