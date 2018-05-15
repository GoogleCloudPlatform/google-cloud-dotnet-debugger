#!/bin/bash

# This script generates coverage report for the c++ code.
# This script assumes that build-deps.sh has already been run.
# A summary page will be printed and a webpage opened for finer grained details about coverage.

set -e

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

if [[ "$OS" == "Windows_NT" ]]
then
  print "Script cannot be run on Windows."
  echo
fi

build=true
while (( "$#" )); do
  if [[ "$1" == "--nobuild" ]]
  then 
    build=false
  fi
  shift
done

COVERAGE_REPORT_DIR="$ROOT_DIR/coverage_report"
INFO_FILE="$COVERAGE_REPORT_DIR/test.info" 
DEBUGGER_ROOT_DIR="$ROOT_DIR/src/google_cloud_debugger"
DEBUGGER_DIR="$DEBUGGER_ROOT_DIR/google_cloud_debugger"
DEBUGGER_LIB_DIR="$DEBUGGER_ROOT_DIR/google_cloud_debugger_lib"
DEBUGGER_TEST_DIR="$DEBUGGER_ROOT_DIR/google_cloud_debugger_test"
DEBUGGER_TEST="$DEBUGGER_TEST_DIR/google_cloud_debugger_test"

rm -r $COVERAGE_REPORT_DIR
mkdir $COVERAGE_REPORT_DIR

if [[ "$build" == true ]]
then   
  make clean -C $DEBUGGER_LIB_DIR
  make clean -C $DEBUGGER_TEST_DIR
  make clean -C $DEBUGGER_DIR
  make -C $DEBUGGER_LIB_DIR COVERAGE=true
  make -C $DEBUGGER_TEST_DIR COVERAGE=true
  make -C $DEBUGGER_DIR COVERAGE=true
fi

$DEBUGGER_TEST
$ROOT_DIR/run_integration_tests.sh

# We pin to a given gcov tool to avoid versioning issues.
lcov --no-external --capture --directory=$DEBUGGER_LIB_DIR --output-file=$INFO_FILE --gcov-tool=gcov-4.4
lcov --remove $INFO_FILE "$DEBUGGER_LIB_DIR/breakpoint.pb.*" --output-file=$INFO_FILE
genhtml --prefix=$DEBUGGER_LIB_DIR --output-directory=$COVERAGE_REPORT_DIR --legend --show-details $INFO_FILE
xdg-open $COVERAGE_REPORT_DIR/index.html
