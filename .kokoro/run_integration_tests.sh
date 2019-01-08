#!/bin/bash

set -e

SCRIPT=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT")
ROOT_DIR=$(dirname "$SCRIPT_DIR")

cd $SCRIPT_DIR
cd ..

# We need to spawn and kill process and need to be root for this to work.
sudo -E su <<EOF

export GOOGLE_APPLICATION_CREDENTIALS="$KOKORO_KEYSTORE_DIR/73609_cloud-sharp-jenkins-compute-service-account"

git submodule init
git submodule update

./build-deps.sh
export LD_LIBRARY_PATH=$ROOT_DIR/third_party/coreclr/bin/Product/Linux.x64.Debug
sudo ldconfig

./build.sh
./run_integration_tests.sh
EOF
