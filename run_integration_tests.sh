#!/bin/bash

# This script runs all integration tests.
# This script assumes that build.sh and build-deps.sh have been run.

set -e

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

AGENT_DIR=$ROOT_DIR/src/Google.Cloud.Diagnostics.Debug

cd $AGENT_DIR

PERFORMANCE_TESTS=false
CONFIG=Debug
while (( "$#" )); do
  if [[ "$1" == "--performance-tests" ]]
  then 
    PERFORMANCE_TESTS=true
  elif [[ "$1" == "--release" ]]
  then
    CONFIG=Release
  fi
  shift
done

export LD_LIBRARY_PATH=$ROOT_DIR/third_party/coreclr/bin/Product/Linux.x64.$CONFIG

dotnet publish $AGENT_DIR/Google.Cloud.Diagnostics.Debug --configuration $CONFIG
dotnet publish $AGENT_DIR/Google.Cloud.Diagnostics.Debug.TestApp --configuration $CONFIG
dotnet test $AGENT_DIR/Google.Cloud.Diagnostics.Debug.IntegrationTests --configuration $CONFIG

if [[ "$PERFORMANCE_TESTS" == true ]]
then
  dotnet test $AGENT_DIR/Google.Cloud.Diagnostics.Debug.PerformanceTests --configuration $CONFIG
fi
