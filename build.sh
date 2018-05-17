#!/bin/bash

# Builds all source in the repo.

set -e

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

AGENT_DIR=$ROOT_DIR/src/Google.Cloud.Diagnostics.Debug
DEBUGGER_DIR=$ROOT_DIR/src/google_cloud_debugger
ANTLR_DIR=$ROOT_DIR/third_party/antlr/lib/cpp

CONFIG=Debug
MAKE_CONFIG_RELEASE=false
REBUILD=
while (( "$#" )); do
  if [[ "$1" == "--release" ]]
  then
    CONFIG=Release
    MAKE_CONFIG_RELEASE=true
  # We have the option to rebuild as make doesn't pick up
  # on the flag change when doing release vs debug so you
  # may need to force it.
  elif [[ "$1" == "--rebuild" ]]
  then 
    REBUILD=--always-make
  fi
  shift
done

dotnet build $AGENT_DIR --configuration $CONFIG

if [[ "$OS" == "Windows_NT" ]]
then
  if ! $(type msbuild &> /dev/null)
  then
    error_message="'msbuild' is not found. Please install 'msbuild' or try "
    error_message+="running this script from a Developer Command Prompt."
    echo $error_message
    exit 1
  fi
  msbuild $DEBUGGER_DIR/google_cloud_debugger.sln //p:Configuration=$CONFIG //p:Platform=x64
else
  make $REBUILD -C $ANTLR_DIR RELEASE=$MAKE_CONFIG_RELEASE
  make $REBUILD -C $DEBUGGER_DIR/google_cloud_debugger_lib RELEASE=$MAKE_CONFIG_RELEASE 
  make $REBUILD -C $DEBUGGER_DIR/google_cloud_debugger RELEASE=$MAKE_CONFIG_RELEASE
  make $REBUILD -C $DEBUGGER_DIR/google_cloud_debugger_test RELEASE=$MAKE_CONFIG_RELEASE
fi
