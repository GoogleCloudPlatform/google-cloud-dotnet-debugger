#!/bin/bash

set -e

SCRIPT=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT")

cd $SCRIPT_DIR
cd ..

# We need to spawn and kill process and need to be root for this to work.
sudo su

./build-deps.sh
./build.sh
./run_unit_tests.sh
