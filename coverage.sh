# This script generates coverage report for the code.
# This script assumes that build-deps.sh has already been run.
# A summary page will be printed and a webpage opened for finer grained details about coverage.
#
# TODO(talarico): Add coverage for C# unit and integration tests.
# TODO(talarico): The application is not shutting down properly so we do not get coverage from integration tests.

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

COVERAGE_REPORT_DIR="$ROOT_DIR/coverage_report"
INFO_FILE="$COVERAGE_REPORT_DIR/test.info" 
DEBUGGER_DIR="$ROOT_DIR/google_cloud_debugger"
DEBUGGER_LIB_DIR="$ROOT_DIR/google_cloud_debugger_lib"
DEBUGGER_TEST_DIR="$ROOT_DIR/google_cloud_debugger_test"
DEBUGGER_TEST="$DEBUGGER_TEST_DIR/google_cloud_debugger_test"

mkdir -p $COVERAGE_REPORT_DIR

#make clean -C $DEBUGGER_LIB_DIR
#make clean -C $DEBUGGER_TEST_DIR
#make clean -C $DEBUGGER_DIR
#make -C $DEBUGGER_LIB_DIR COVERAGE=true
#make -C $DEBUGGER_TEST_DIR COVERAGE=true
#make -C $DEBUGGER_DIR COVERAGE=true

#$DEBUGGER_TEST
#$ROOT_DIR/run_integration_tests.sh

# We pin to a given gcov tool to avoid versioning issues.
lcov --no-external --capture --directory=$DEBUGGER_LIB_DIR --output-file=$INFO_FILE --gcov-tool=gcov-4.4
genhtml --prefix=$DEBUGGER_LIB_DIR --output-directory=$COVERAGE_REPORT_DIR --legend --show-details $INFO_FILE
xdg-open $COVERAGE_REPORT_DIR/index.html
