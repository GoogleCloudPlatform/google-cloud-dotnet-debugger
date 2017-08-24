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

  // First argument to the test app will be the process ID of the
  // process we want to debug.
  Debugger debugger;

  string app_path(argv[1]);

  string command_line = "dotnet " + app_path;

  std::vector<WCHAR> result;
  ConvertStringToWCharPtr(command_line, &result);

  hr = debugger.StartDebugging(result);
  if (FAILED(hr)) {
    cerr << "HR Failed";
    return -1;
  }

  // This will launch an infinite while loop to wait and read.
  // TODO(quoct): We may want to do this in a different thread
  // so that there is a way to quit this test app.

  std::thread sync_breakpoints_thread([](Debugger *debugger) {
    debugger->SyncBreakpoints();
  }, &debugger);

  string input_string;
  string quit = "quit";
  while (true) {
    // TODO(quoct): Seems like we have to feed the input in twice
    // for getline to work. How to fix this?
    getline(cin, input_string);
    cin.clear();
    cin.sync();

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

