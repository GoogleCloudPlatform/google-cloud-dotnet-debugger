#!/bin/bash

set -e

SCRIPT=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT")

cd $SCRIPT_DIR
cd ..

sudo ldconfig
git submodule init
git submodule update

./build-deps.sh
./build.sh
./run_unit_tests.sh
