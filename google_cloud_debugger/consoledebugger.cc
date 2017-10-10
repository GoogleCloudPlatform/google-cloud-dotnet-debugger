// ConsoleApplication1.cpp : Defines the entry point for the console
// application.

// TODO: Add cleanup to release pointer.

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "debugger.h"
#include "winerror.h"

#include "dbgobject.h"

using cxxopts::Options;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::Debugger;
using std::cerr;
using std::cin;
using std::endl;
using std::hex;
using std::string;

// If given this option, the debugger will not perform property evaluation.
const string kEvaluationOption = "property-evaluation";

// If given this option, the debugger will start the application using this
// path.
const string kApplicationPathOption = "application-path";

// If given this option, the debugger will attach to the running application
// with this process ID.
const string kApplicationIDOption = "application-id";

int main(int argc, char *argv[]) {
  Options options("GoogleCloudConsoleDebugger",
                  "Google Cloud Console Debugger for .NET");

  bool property_evaluation = false;
  string app_path;
  uint32_t app_id;

  options.add_options()(kApplicationPathOption,
                        "Path to the application. If used, the debugger will "
                        "start the application using this path.",
                        cxxopts::value<std::string>(app_path))(
      kApplicationIDOption,
      "Process ID of the application to be debugged. If used, the debugger "
      "will attach to the running application using this ID.",
      cxxopts::value<std::uint32_t>(app_id))(
      kEvaluationOption,
      "If used, the debugger will attempt to evaluate property of classes by. "
      "This may modify the state of the application.",
      cxxopts::value<bool>(property_evaluation));

  options.parse(argc, argv);

  // Cannot supply both path and ID to the option.
  if (options.count(kApplicationPathOption) &&
      options.count(kApplicationIDOption)) {
    cerr << "The debugger can only take in either the application path or the "
            "process ID of the running application, not both.";
    return -1;
  }

  // Missing either path or ID.
  if (!options.count(kApplicationPathOption) &&
      !options.count(kApplicationIDOption)) {
    cerr << "Either the application path or the process ID of the running "
            "application has to be given to the debugger.";
    return -1;
  }

  Debugger debugger;
  HRESULT hr;

  if (options.count(kApplicationPathOption)) {
    // string app_path = options[kApplicationPathOption].as<std::string>();
    string command_line = "dotnet " + app_path;
    std::vector<WCHAR> wchar_command_line =
        ConvertStringToWCharPtr(command_line);

    if (wchar_command_line.size() == 0) {
      cerr << "Application's name is not valid." << endl;
      return -1;
    }

    hr = debugger.StartDebugging(wchar_command_line);
  } else {
    // uint32_t app_id = options[kApplicationIDOption].as<std::uint32_t>();
    hr = debugger.StartDebugging(app_id);
  }

  if (FAILED(hr)) {
    cerr << "Debugger fails with HRESULT " << hex << hr << endl;
    return -1;
  }

  // Sets property evaluation.
  debugger.SetPropertyEvaluation(property_evaluation);

  // This will launch an infinite while loop to wait and read.
  // When the server connection of the named pipe breaks, the loop
  // will be broken and the application process will be terminated
  // in the debugger's destructor.
  debugger.SyncBreakpoints();

  return 0;
}
