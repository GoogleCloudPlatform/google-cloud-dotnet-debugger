#!/bin/bash

# This script runs all integration tests.
# This script assumes that build.sh and build-deps.sh have been run.

set -e

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

AGENT_DIR=$ROOT_DIR/src/Google.Cloud.Diagnostics.Debug

PERFORMANCE_TESTS=false
LONG_RUNNING_TESTS=false
INTEGRATION_TESTS=true
CONFIG=Debug

while (( "$#" )); do
  if [[ "$1" == "--performance-tests" ]]
  then 
    PERFORMANCE_TESTS=true
  elif [[ "$1" == "--skip-integration-tests" ]]
  then
    INTEGRATION_TESTS=false
  elif [[ "$1" == "--long-running-tests" ]]
  then
    LONG_RUNNING_TESTS=true
  elif [[ "$1" == "--release" ]]
  then
    CONFIG=Release
  fi
  shift
done

if [[ "$OS" != "Windows_NT" ]]
then
  export LD_LIBRARY_PATH=$ROOT_DIR/third_party/coreclr/bin/Product/Linux.x64.$CONFIG
fi

dotnet publish $AGENT_DIR/Google.Cloud.Diagnostics.Debug --configuration $CONFIG
dotnet publish $AGENT_DIR/Google.Cloud.Diagnostics.Debug.TestApp --configuration $CONFIG

if [[ "$INTEGRATION_TESTS" == true ]]
then
  dotnet test $AGENT_DIR/Google.Cloud.Diagnostics.Debug.IntegrationTests --configuration $CONFIG
fi

if [[ "$PERFORMANCE_TESTS" == true ]]
then
  dotnet test $AGENT_DIR/Google.Cloud.Diagnostics.Debug.PerformanceTests --configuration $CONFIG
fi

if [[ "$LONG_RUNNING_TESTS" == true ]]
then
  dotnet test $AGENT_DIR/Google.Cloud.Diagnostics.Debug.LongRunningTests --configuration $CONFIG
fi
