// ConsoleApplication1.cpp : Defines the entry point for the console
// application.

// TODO: Add cleanup to release pointer.

#include <iostream>
#include <string>
#include <thread>
#include "debugger.h"
using namespace std;
using namespace google_cloud_debugger;

const static std::string quit = "quit";

int main(int argc, char *argv[]) {
  HRESULT hr;
  string input_string;
  int procId;
  string input;

  getline(cin, input);
  procId = stoi(argv[1]);
  cout << "input is " << input << " for process ID " << procId;

  Debugger debugger;

  if (input.compare(quit) == 0) {
    return 0;
  }

  hr = debugger.StartDebugging(procId);
  if (FAILED(hr)) {
    cout << "HR Failed";
    return -1;
  }

  debugger.SyncBreakpoints();

  /*
  std::thread sync_thread([](Debugger &debugger) {
    debugger.SyncBreakpoints();
  }, debugger);

  while (true) {
    cin.clear();
    getline(cin, input);
    if (input.compare(quit) == 0) {
      sync_thread.join();
      break;
    }
    std::this_thread::sleep_for(2s);
  }
  */
  return 0;
}
