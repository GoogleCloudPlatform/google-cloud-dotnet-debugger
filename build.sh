#!/bin/bash

# Builds all source in the repo.
# TODO(talarico): Allow configuration of platform and config

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

AGENT_DIR=$ROOT_DIR/src/Google.Cloud.Diagnostics.Debug
DEBUGGER_DIR=$ROOT_DIR/src/google_cloud_debugger

configuration=Debug
while (( "$#" )); do
  if [[ "$1" == "--release" ]]
  then
    configuration=Release
  fi
  shift
done

dotnet build $AGENT_DIR --configuration $configuration

if [[ "$OS" == "Windows_NT" ]]
then
  if ! $(type msbuild &> /dev/null)
  then
    error_message="'msbuild' is not found. Please install 'msbuild' or try "
    error_message+="running this script from a Developer Command Prompt."
    echo $error_message
    exit 1
  fi
  echo $configuration
  msbuild $DEBUGGER_DIR/google_cloud_debugger.sln //p:Configuration=$configuration //p:Platform=x64
else
  make -C $DEBUGGER_DIR/google_cloud_debugger_lib
  make -C $DEBUGGER_DIR/google_cloud_debugger
  make -C $DEBUGGER_DIR/google_cloud_debugger_test
fi
