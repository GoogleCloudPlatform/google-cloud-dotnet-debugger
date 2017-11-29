# This script generates coverage report for the code.
# This script assumes that build-deps.sh has already been run.
# A summary page will be printed and a webpage opened for finer grained details about coverage.
#
# TODO(talarico): This script will need to be updated when we switch to codecov.
# TODO(talarico): Add coverage for C++ from integration tests.
# TODO(talarico): Add coverage for C# unit and integration tests.

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

COVERAGE_REPORT_DIR="$ROOT_DIR/coverage_report"
LLVM_PROFILE_FILE="$COVERAGE_REPORT_DIR/default.profraw"
LLVM_PROFILE_DATA_FILE="$COVERAGE_REPORT_DIR/default.profdata"
DEBUGGER_LIB_DIR="$ROOT_DIR/google_cloud_debugger_lib"
DEBUGGER_TEST_DIR="$ROOT_DIR/google_cloud_debugger_test"
DEBUGGER_TEST="$ROOT_DIR/google_cloud_debugger_test/google_cloud_debugger_test"

mkdir -p $COVERAGE_REPORT_DIR

make clean -C $DEBUGGER_LIB_DIR
make clean -C $DEBUGGER_TEST_DIR

make -C $DEBUGGER_LIB_DIR COVERAGE=true
make -C $DEBUGGER_TEST_DIR COVERAGE=true

$DEBUGGER_TEST

llvm-profdata-3.9 merge -sparse $LLVM_PROFILE_FILE -o $LLVM_PROFILE_DATA_FILE
llvm-cov-3.9 show $DEBUGGER_TEST -instr-profile=$LLVM_PROFILE_DATA_FILE -format=html -output-dir=$COVERAGE_REPORT_DIR
llvm-cov-3.9 report $DEBUGGER_TEST -instr-profile=$LLVM_PROFILE_DATA_FILE

xdg-open $COVERAGE_REPORT_DIR/index.html
