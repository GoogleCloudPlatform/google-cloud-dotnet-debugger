#!/bin/bash

# This script generates coverage report for the C# code.
# This script assumes that build-deps.sh has already been run.
# A summary page will be printed and a webpage opened for finer grained details about coverage.

set -e

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

if [[ "$OS" != "Windows_NT" ]]
then
  print "Script can only be run on Windows."
  echo
fi

COVERAGE_REPORT_DIR="$ROOT_DIR/coverage_report"
PACKAGES_DIR="$COVERAGE_REPORT_DIR/packages"
COVERAGE_REPORT="$COVERAGE_REPORT_DIR/coverage.xml"
OPEN_COVER="$PACKAGES_DIR/OpenCover.4.6.519/tools/OpenCover.Console.exe"
REPORT_GENERATOR="$PACKAGES_DIR/ReportGenerator.3.0.2/tools/ReportGenerator.exe"
AGENT_DIR="$ROOT_DIR/src/Google.Cloud.Diagnostics.Debug"

rm -r $COVERAGE_REPORT_DIR
mkdir $COVERAGE_REPORT_DIR
mkdir $PACKAGES_DIR

nuget install OpenCover -Version 4.6.519 -OutputDirectory $PACKAGES_DIR
nuget install ReportGenerator -Version 3.0.2 -OutputDirectory $PACKAGES_DIR

dotnet publish $AGENT_DIR/Google.Cloud.Diagnostics.Debug.TestApp

# Exclude all tests and generated protobuf files from the report.
$OPEN_COVER -register:user -target:"C:/Program Files/dotnet/dotnet.exe" -targetargs:test  -targetdir:"$AGENT_DIR" -output:"$COVERAGE_REPORT" -oldstyle -filter:"+[*]* -[*.Tests]* -[*.IntegrationTests]* -[*.PerformanceTests]* -[*.TestApp]* -[Google.Cloud.Diagnostics.Debug]*Breakpoint -[Google.Cloud.Diagnostics.Debug]*BreakpointReflection -[Google.Cloud.Diagnostics.Debug]*SourceLocation -[Google.Cloud.Diagnostics.Debug]*StackFrame -[Google.Cloud.Diagnostics.Debug]*Status -[Google.Cloud.Diagnostics.Debug]*Variable"

$REPORT_GENERATOR -reports:$COVERAGE_REPORT -targetdir:$COVERAGE_REPORT_DIR

start $COVERAGE_REPORT_DIR/index.htm