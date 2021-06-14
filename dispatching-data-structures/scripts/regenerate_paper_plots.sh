#!/bin/bash

critical() {
    echo "$1"
    exit 1
}

SCRIPTPATH="$( cd "$(dirname "$0")" > /dev/null 2>&1 || critical 'Could not determine script path.' ; pwd -P )"

available_ram=$(free -m | awk '/^Mem/ {print $7}')
if [ "$available_ram" -lt 13000 ]; then
    read -r -n1 -s -p "Because of how google benchmark works, this will consume up to 13 GB of RAM. You do not have this much RAM available. It might still work if there is enough swap to move other stuff to. Or your PC will freeze and die. Are you sure you want to continue? [y/N]" decision
    echo ""
    if [ "$decision" != "y" ] && [ "$decision" != "Y" ]; then
        exit 0
    fi
fi

# Create result directory
mkdir -p results || critical "Could not create result directory."
cd results

# Run measurements
dispatching_time_json=$(python3 "$SCRIPTPATH"/do_measurements.py "$SCRIPTPATH"/../build/benchmark)
cache_misses_json=$(python3 "$SCRIPTPATH"/do_measurements.py "$SCRIPTPATH"/../build/cache_analyzer)

# Print the jsons.
if command -v jq > /dev/null 2>&1; then
    echo "$dispatching_time_json" | jq > dispatching_time.json
    echo "$cache_misses_json" | jq > cache_misses.json
else
    echo "$dispatching_time_json" > dispatching_time.json
    echo "$cache_misses_json" > cache_misses.json
fi

# Generate plots
python3 "$SCRIPTPATH"/makePlot.py DispatchTimesPerMapping -m zeek -r dispatching_time.json -o zeek.png
python3 "$SCRIPTPATH"/makePlot.py DispatchTimesPerMapping -m fragmented -r dispatching_time.json -o fragmented.png
python3 "$SCRIPTPATH"/makePlot.py CacheMissesCIC_IDS -m zeek -r cache_misses.json -o cache_misses.png
