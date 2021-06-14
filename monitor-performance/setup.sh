#!/bin/bash

ZEEK_ORIG_COMMIT="f744d4c070b324dc74b5570b54499c11127bc3a0"
ZEEK_EXTD_COMMIT="68e20def1e78e7e554ea903bb49e904d126c3071"

critical() {
    echo "$1"
    exit 1
}

BASE_DIR="$( cd "$(dirname "$0")" > /dev/null 2>&1 || critical 'Could not determine base dir.' ; pwd -P )"
NUM_CORES=$(($(nproc --all || echo 2) - 1))

CSET_BINARY="${BASE_DIR}/cpuset/cset"

cd $BASE_DIR

# Clone Zeek
echo "Cloning Zeek..."
git clone --recursive https://github.com/zeek/zeek.git zeek-original
cp -r zeek-original zeek-extended

# Build Zeek versions
echo "Building original Zeek..."
(
	cd zeek-original ;
	git checkout $ZEEK_ORIG_COMMIT ;
	git submodule update --recursive --init ;
	./configure --enable-jemalloc --build-type=Release ;
	make -j $NUM_CORES
)
echo "Building extended Zeek..."
(
	cd zeek-extended ;
	git checkout $ZEEK_EXTD_COMMIT ;
	git submodule update --recursive --init ;
	./configure --enable-jemalloc --build-type=Release ;
	make -j $NUM_CORES
)

# Clone and build cset
echo "Clone and build cset..."
(
	git clone https://github.com/lpechacek/cpuset.git ;
	cd cpuset ;
	./setup.py build
)
