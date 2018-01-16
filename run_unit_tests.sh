#!/bin/bash

# This script runs all unit tests.
# This script assumes that build.sh and build-deps.sh have been run.
#
# TODO(talarico): Make sure this works with debug and release builds

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

AGENT_DIR=$ROOT_DIR/src/Google.Cloud.Diagnostics.Debug
DEBUGGER_DIR=$ROOT_DIR/src/google_cloud_debugger

dotnet test $AGENT_DIR/Google.Cloud.Diagnostics.Debug.Tests

if [[ "$OS" == "Windows_NT" ]]
then
  $DEBUGGER_DIR/x64/Debug/google_cloud_debugger_test
else
  $DEBUGGER_DIR/google_cloud_debugger_test/google_cloud_debugger_test
fi
