#!/bin/bash
# ================= compare_runtimes.sh =================
# Convenience script to compare custom runtime with OpenCilk

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "======================================"
echo "Runtime Comparison: Custom vs OpenCilk"
echo "======================================"
echo ""

# Check if build directory exists
if [ ! -d "build" ]; then
    echo -e "${RED}Error: build directory not found${NC}"
    echo "Please run 'mkdir build && cd build && cmake .. && make' first"
    exit 1
fi

# Check if custom runtime executable exists
if [ ! -f "build/fib" ]; then
    echo -e "${RED}Error: build/fib not found${NC}"
    echo "Please build the project first: cd build && make"
    exit 1
fi

# Check if OpenCilk version exists
if [ ! -f "build/fib_cilk" ]; then
    echo -e "${YELLOW}Warning: build/fib_cilk not found${NC}"
    echo "OpenCilk version is not built. This could be because:"
    echo "  1. OpenCilk is not installed on your system"
    echo "  2. The build was not configured to detect OpenCilk"
    echo ""
    echo "To install OpenCilk, visit: https://opencilk.org"
    echo ""
    echo "Running comparison with custom runtime only..."
    python3 scripts/run_thread_exp.py --app fib
    exit 0
fi

echo -e "${GREEN}Both executables found!${NC}"
echo "  - Custom Runtime: build/fib"
echo "  - OpenCilk:       build/fib_cilk"
echo ""

# Run the comparison
echo "Running comparison tests..."
echo "This will test with 8, 16, 32, and 64 threads"
echo ""

python3 scripts/run_thread_exp.py --app fib --baseline build/fib_cilk

echo ""
echo -e "${GREEN}Comparison complete!${NC}"
echo ""
echo "Results saved to:"
echo "  - temp/fib/latest/performance_report.md (detailed comparison)"
echo "  - temp/fib/comparison_report.md (historical comparison)"
echo ""
