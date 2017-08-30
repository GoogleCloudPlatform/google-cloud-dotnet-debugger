// ConsoleApplication1.cpp : Defines the entry point for the console
// application.

// TODO: Add cleanup to release pointer.

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "debugger.h"
#include "winerror.h"

#include "dbgobject.h"

using std::cerr;
using std::cin;
using std::endl;
using std::hex;
using std::string;
using google_cloud_debugger::Debugger;
using google_cloud_debugger::ConvertStringToWCharPtr;

const string kEvaluationOption = "--property-evaluation";

int main(int argc, char *argv[]) {
  if (argc == 1) {
    cerr << "Debugger needs at least the path to the application." << endl;
    return E_INVALIDARG;
  }

  HRESULT hr;

  // First argument to the test app will be the full path to the
  // process we want to debug.
  // Second argument is set if we need to perform property evaluation.
  Debugger debugger;
  string app_path(argv[1]);
  string command_line = "dotnet " + app_path;
  std::vector<WCHAR> result = ConvertStringToWCharPtr(command_line);

  if (result.size() == 0) {
    cerr << "Application's name is not valid." << endl;
    return -1;
  }

  hr = debugger.StartDebugging(result);
  if (FAILED(hr)) {
    cerr << "Debugger fails with HRESULT " << hex << hr << endl;
    return -1;
  }

  // Checks for property evaluation.
  if (argc == 3) {
    string evaluation(argv[2]);
    if (kEvaluationOption.compare(evaluation) == 0) {
      // Turns on property evaluation.
      debugger.SetPropertyEvaluation(TRUE);
    }
  }

  // This will launch an infinite while loop to wait and read.
  // When the server connection of the named pipe breaks, the loop
  // will be broken and the application process will be terminated
  // in the debugger's destructor.
  debugger.SyncBreakpoints();

  return 0;
}
