#!/bin/bash

# This script runs all unit tests.
# This script assumes that build.sh and build-deps.sh have been run.

set -e

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

AGENT_DIR=$ROOT_DIR/src/Google.Cloud.Diagnostics.Debug
DEBUGGER_DIR=$ROOT_DIR/src/google_cloud_debugger

CONFIG=Debug
while (( "$#" )); do
  if [[ "$1" == "--release" ]]
  then
    CONFIG=Release
  fi
  shift
done

dotnet test $AGENT_DIR/Google.Cloud.Diagnostics.Debug.Tests --configuration $CONFIG

if [[ "$OS" == "Windows_NT" ]]
then
  $DEBUGGER_DIR/x64/$CONFIG/google_cloud_debugger_test
else
  $DEBUGGER_DIR/google_cloud_debugger_test/google_cloud_debugger_test
fi
