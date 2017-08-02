// ConsoleApplication1.cpp : Defines the entry point for the console
// application.

// TODO: Add cleanup to release pointer.

#include <iostream>
#include <string>
#include <thread>
#include "debugger.h"
using namespace std;
using namespace google_cloud_debugger;

// This is a simple test app to test out the debugger.
// This is only for proof of concept.
int main(int argc, char *argv[]) {
  HRESULT hr;
  string input_string;
  int procId;
  string input;

  getline(cin, input);
  // First argument to the test app will be the process ID of the
  // process we want to debug.
  procId = stoi(argv[1]);
  cout << "input is " << input << " for process ID " << procId;

  Debugger debugger;

  hr = debugger.StartDebugging(procId);
  if (FAILED(hr)) {
    cout << "HR Failed";
    return -1;
  }

  // This will launch an infinite while loop to wait and read.
  // TODO(quoct): We may want to do this in a different thread
  // so that there is a way to quit this test app.
  debugger.SyncBreakpoints();

  return 0;
}
