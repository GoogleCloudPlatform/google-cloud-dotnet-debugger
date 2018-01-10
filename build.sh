#!/bin/bash

# Builds all source in the repo.
# TODO(talarico): Allow configuration of platform and config

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

dotnet build $ROOT_DIR

if [[ "$OS" == "Windows_NT" ]]
then
  if ! $(type msbuild &> /dev/null)
  then
    error_message="'msbuild' is not found. Please install 'msbuild' or try "
    error_message+="running this script from a Developer Command Prompt."
    echo $error_message
    exit 1
  fi
  msbuild $ROOT_DIR/GoogleCloudDebugger.sln //p:Configuration=Debug //p:Platform=x64
else
  make -C $ROOT_DIR/google_cloud_debugger_lib
  make -C $ROOT_DIR/google_cloud_debugger
  make -C $ROOT_DIR/google_cloud_debugger_test
fi
