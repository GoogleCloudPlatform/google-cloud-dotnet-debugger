#!/bin/bash

# Builds all source in the repo.
# TODO(talarico): Allow configuration of platform and config

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

if [[ "$OS" == "Windows_NT" ]]
then
  if ! $(type devenv &> /dev/null)
  then
    error_message="'devenv' is not found. Please install 'devenv' or try "
    error_message+="running this script from a Developer Command Prompt."
    echo $error_message
    exit 1
  fi
  devenv $ROOT_DIR/GoogleCloudDebugger.sln //Build
else
  make -C $ROOT_DIR/google_cloud_debugger_lib
  make -C $ROOT_DIR/google_cloud_debugger
  make -C $ROOT_DIR/google_cloud_debugger_test
  dotnet build $ROOT_DIR
fi
