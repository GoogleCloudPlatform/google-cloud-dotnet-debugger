#!/bin/bash

set -e

SCRIPT=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT")

cd $SCRIPT_DIR
cd ..

# We need to spawn and kill process and need to be root for this to work.
sudo -E su <<EOF

sudo ldconfig
git submodule init
git submodule update

./build-deps.sh
./build.sh
./run_unit_tests.sh
EOF
