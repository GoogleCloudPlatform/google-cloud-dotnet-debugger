# Google Cloud Debugger for .NET Core
[![Travis build Status](https://travis-ci.com/GoogleCloudPlatform/google-cloud-dotnet-debugger.svg?token=uPVZj7upLKBYvMVpisAp&branch=master)](https://travis-ci.com/GoogleCloudPlatform/google-cloud-dotnet-debugger)

.NET Core Debugger for the [Stackdriver Debugger](https://cloud.google.com/debugger/).

Stackdriver Debugger is a feature of the Google Cloud Platform that lets you inspect the state
of an application at any code location without using logging statements and without stopping or
slowing down your applications. 

## Overview
The .NET Core Stackdriver Debugger is comprised of four major parts

* **Debuggee** - The _Debuggee_ is the .NET Core application being debugged.
* **Stackdriver Debugger** - The _Stackdriver Debugger_ is a web application that allows
users to view source code and set snapshots on _Debuggees_.
* **.NET Debugger** - The _.NET Core Debugger_ is an implementation of the 
[`ICorDebug`](https://docs.microsoft.com/en-us/dotnet/framework/unmanaged-api/debugging/icordebug-interface)
interface used to debug the _Debugee_.
* **Debugging Agent** - The _Debugging Agent_ will run alongside the _.NET Core Debugger_.  It is responsible for
starting the _.NET Core Debugger_ as well as maintaining communication with the _Stackdriver Debugger_.

## Building

Before building ensure you have all of the dependencies installed to build:
* [dotnet/coreclr](https://github.com/dotnet/coreclr)
* [google/googletest](https://github.com/google/googletest)
* [google/protobuf](https://github.com/google/protobuf)

### Linux

The repository and dependencies can be built with the following:
  ```
  ./build-deps.sh
  ./build.sh
  ``` 

### Windows

The dependencies can be built with the following:
  ```
  .\build-deps.cmd
  .\build.sh
  ``` 

## Running

Once the repository and dependencies are built you can run the debugger with:

### Linux
  ```
  dotnet ./Google.Cloud.Diagnostics.Debug/bin/Debug/netcoreapp1.1/Google.Cloud.Diagnostics.Debug.dll \
    --debugger=./google_cloud_debugger/google_cloud_debugger \
    --application-path=<Path to a .NET Core application> \
    --project-id=<Google Cloud Console project id> \
    --module=<Module of your application> \
    --version=<Version of your application>
  ```

### Windows
  ```
  dotnet .\Google.Cloud.Diagnostics.Debug\bin\Debug\netcoreapp1.1\Google.Cloud.Diagnostics.Debug.dll ^
    --debugger=.\x64\Debug\google_cloud_debugger.exe ^
    --application-path=<Path to a .NET Core application> ^
    --project-id=<Google Cloud Console project id> ^
    --module=<Module of your application> ^
    --version=<Version of your application>
  ```


## Running Tests

Once the repository and dependencies are built you can run the tests with:

### Unit Tests
  ```
  ./run_unit_tests.sh
  ```

### Integration Tests
  ```
  ./run_integration_tests.sh
  ```

### Performance Tests
  ```
  ./run_integration_tests.sh --performance-tests
  ```