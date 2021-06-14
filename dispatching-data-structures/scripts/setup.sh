#!/bin/bash

critical() {
    echo "$1"
    exit 1
}

SCRIPTPATH="$( cd "$(dirname "$0")" > /dev/null 2>&1 || critical 'Could not determine script path.' ; pwd -P )"
NUM_CORES=$(($(nproc --all || echo 2) - 1))
RUNS=10

if ! command -v taskset > /dev/null 2>&1; then
    critical "taskset is missing on your system. Please install it."
fi

cd "$SCRIPTPATH/.." || critical "Could not cd into project root."

# Generate artifical packet trace files
echo "Generating $RUNS artificial packet trace files..."
mkdir -p input/traces
python3 scripts/gen_packet.py random_ex -c $RUNS -o input/traces/rand || critical "Could not generate artificial packet files."

# Generate analyzer mappings
echo "Generating analyzers..."
mkdir -p input/analyzers
python3 scripts/gen_analyzer.py fragmented -o input/analyzers/fragmented || critical "Could not generate analyzer mappings."
python3 scripts/gen_analyzer.py zeek -o input/analyzers/zeek || critical "Could not generate analyzer mappings."

# Generate metaprogramming code
echo "Generating metaprogramming code..."
python3 scripts/gen_code.py all || critical "Could not generate metaprogramming code."

# Make the project
echo "Making project..."
mkdir -p build
cd build || critical "Could not cd into build directory."
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .. || critical "CMake for the project failed."
make -j "$NUM_CORES" || critical "Making the project failed."

echo "Generating measurements..."

# Ensure that applications are executable
chmod +x benchmark
chmod +x cache_analyzer

# Check PAPI performance counters
if ! ./cache_analyzer papi_check > /dev/null 2>&1; then
    critical "The necessary PAPI performance counters are not available on your system. Check ./build/papi/papi-install/bin/papi_component_avail for info why that might be the case."
fi
