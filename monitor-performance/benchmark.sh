#!/bin/bash

# 1,3
ZEEK_CPU=1
TRACE="$1"
RUNS=10
STATS_FILE="stats.json"

critical() {
    echo "$1"
    exit 1
}

BASE_DIR="$( cd "$(dirname "$0")" > /dev/null 2>&1 || critical 'Could not determine base dir.' ; pwd -P )"

ZEEK_ORIG_BUILD="${BASE_DIR}/zeek-original/build"
ZEEK_EXTD_BUILD="${BASE_DIR}/zeek-extended/build"
ZEEK_SEED_FILE="${BASE_DIR}/random.seed"
CSET_BINARY="${BASE_DIR}/cpuset/cset"


do_avgtime_benchmark() {
	ZEEK_BUILD=$1
	ZEEK_BIN="${ZEEK_BUILD}/src/zeek"

	export ZEEKPATH=`${ZEEK_BUILD}/zeek-path-dev`
	export ZEEK_PLUGIN_PATH=

	# Avgtime: Discard first run, 30 runs
	# Zeek: Read $TRACE, disable log writing
	${CSET_BINARY} shield --exec avgtime -- -d -r30 \
		${ZEEK_BIN} -r $TRACE \
		--load-seeds ${ZEEK_SEED_FILE} \
		#"Log::default_writer = Log::WRITER_NONE"
}

do_time_benchmark_run() {
	ZEEK_BUILD=$1
	OUT_FILE=$2
	RUN=$3
	ZEEK_BIN="${ZEEK_BUILD}/src/zeek"

	export ZEEKPATH=`${ZEEK_BUILD}/zeek-path-dev`
	export ZEEK_PLUGIN_PATH=

	# Time: JSON-like format, append to OUT_FILE
	# Zeek: Read $TRACE, disable log writing
	${CSET_BINARY} shield --exec time -- \
		-f "{\n\"run\": ${RUN},\n\"time\": %e,\n\"memory\": %M\n}" \
		-a -o $OUT_FILE \
		${ZEEK_BIN} -r $TRACE \
		--load-seeds ${ZEEK_SEED_FILE} \
		#"Log::default_writer = Log::WRITER_NONE"
}

do_time_benchmark() {
	ZEEK_BUILD=$1
	OUT_FILE=$2
	
	echo "[" > $OUT_FILE
	# Execute runs
	for ((i=1; i<=$RUNS; i++));	do
		do_time_benchmark_run $ZEEK_BUILD $OUT_FILE $i
		# Separate JSON elements
		if [ $i -ne $RUNS ] ; then
			echo "," >> $OUT_FILE
		fi
	done
	echo "]" >> $OUT_FILE
}

cd $BASE_DIR

# Shield CPU
${CSET_BINARY} shield --cpu $ZEEK_CPU --kthread=on

mkdir -p results
cd results

# Execute benchmarks
( mkdir orig ; cd orig ; do_time_benchmark $ZEEK_ORIG_BUILD $STATS_FILE )
( mkdir extd ; cd extd ; do_time_benchmark $ZEEK_EXTD_BUILD $STATS_FILE )

# Benchmarks using avgtime (time only, more precise, 30 runs)
#( mkdir orig ; cd orig ; do_avgtime_benchmark $ZEEK_ORIG_BUILD )
#( mkdir extd ; cd extd ; do_avgtime_benchmark $ZEEK_EXTD_BUILD )

# Reset isolation
${CSET_BINARY} shield --reset

cd results
python3 ../stats.py
