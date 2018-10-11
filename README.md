# Stackdriver Debugger for .NET Core - [Alpha](https://cloud.google.com/terms/launch-stages)
[![release level](https://img.shields.io/badge/release%20level-alpha-orange.svg?style&#x3D;flat)](https://cloud.google.com/terms/launch-stages)
[![Travis Build Status](https://travis-ci.com/GoogleCloudPlatform/google-cloud-dotnet-debugger.svg?token=uPVZj7upLKBYvMVpisAp&branch=master)](https://travis-ci.com/GoogleCloudPlatform/google-cloud-dotnet-debugger)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/GoogleCloudPlatform/google-cloud-dotnet-debugger?branch=master&svg=true)](https://ci.appveyor.com/project/GoogleCloudPlatform/google-cloud-dotnet-debugger)

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

## Docker Images

Prebuilt [Ubuntu 16.04 (x64) Docker images](https://gcr.io/dotnet-debugger/aspnetcore) are available for use.
For detailed instructions see the [Stackdriver Debugger documentation for .NET Core](https://cloud.google.com/debugger/docs/setup/dotnet).

* [gcr.io/dotnet-debugger/aspnetcore:1.0](http://gcr.io/dotnet-debugger/aspnetcore:1.0)
* [gcr.io/dotnet-debugger/aspnetcore:1.1](http://gcr.io/dotnet-debugger/aspnetcore:1.1)
* [gcr.io/dotnet-debugger/aspnetcore:2.0](http://gcr.io/dotnet-debugger/aspnetcore:2.0)
* [gcr.io/dotnet-debugger/aspnetcore:2.1](http://gcr.io/dotnet-debugger/aspnetcore:2.1)


## Usage

The .NET Core Stackdriver Debugger is currently in Alpha.
This product might be changed in backward-incompatible ways and is not subject to any SLA or deprecation policy.
It is not guaranteed to move forward to General Availability.
This product is not intended for real-time usage in critical applications.

For detailed usage instructions see the official [Stackdriver Debugger for .NET Core documentation](https://cloud.google.com/debugger/docs/setup/dotnet).

To be able to use the .NET Core Stackdriver Debugger you will need to deploy PDBs
with your code.  See the example below:

```
...
  <PropertyGroup>
    <TargetFramework>netcoreapp2.0</TargetFramework>
    <DebugType>portable</DebugType>
  </PropertyGroup>
...
```

Note: When debugging a `Release` build variables may be misnamed or missing.

## Compatibility

This debugger has been tested with the following .NET Core versions:
* .NET Core 1.0.x
* .NET Core 1.1.x
* .NET Core 2.0.x
 
This debugger has been tested on the following runtimes:
* win-x64
* debian.8-x64
* ubuntu.16.04-x64

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
  dotnet ./Google.Cloud.Diagnostics.Debug/bin/Debug/netcoreapp2.0/Google.Cloud.Diagnostics.Debug.dll \
    --debugger=./google_cloud_debugger/google_cloud_debugger \
    --application-id=<Process ID of a running .NET Core application to debug> \
    --project-id=<Google Cloud Console project id> \
    --module=<Module of your application> \
    --version=<Version of your application>
  ```

### Windows
  ```
  dotnet .\Google.Cloud.Diagnostics.Debug\bin\Debug\netcoreapp2.0\Google.Cloud.Diagnostics.Debug.dll ^
    --debugger=.\x64\Debug\google_cloud_debugger.exe ^
    --application-id=<Process ID of a running .NET Core application to debug> ^
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