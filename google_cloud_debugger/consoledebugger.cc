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
using std::string;
using google_cloud_debugger::Debugger;
using google_cloud_debugger::ConvertStringToWCharPtr;

// This is a simple test app to test out the debugger.
// This is only for proof of concept.
int main(int argc, char *argv[]) {
  HRESULT hr;

  // First argument to the test app will be the full path to the
  // process we want to debug.
  Debugger debugger;

  string app_path(argv[1]);

  string command_line = "dotnet " + app_path;

  std::vector<WCHAR> result = ConvertStringToWCharPtr(command_line);
  if (result.size() == 0) {
    cerr << "Application's name is not valid.";
    return -1;
  }

  hr = debugger.StartDebugging(result);
  if (FAILED(hr)) {
    cerr << "HR Failed";
    return -1;
  }

  // This will launch an infinite while loop to wait and read.
  // That's why we launch it in a different thread so we can
  // break out of the loop from this thread.
  std::thread sync_breakpoints_thread([](Debugger *debugger) {
    debugger->SyncBreakpoints();
  }, &debugger);

  string input_string;

  // Quits the debugger if this string is supplied.
  const string quit = "quit";

  while (true) {
    cin >> input_string;
    if (quit.compare(input_string) == 0) {
      hr = debugger.CancelSyncBreakpoints();
      if (FAILED(hr)) {
        cerr << "Failed to break out of sync breakpoints." << endl;
      }
      sync_breakpoints_thread.join();
      return 0;
    }
  }

  return 0;
}
