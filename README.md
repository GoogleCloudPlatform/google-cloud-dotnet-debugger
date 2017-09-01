# Google Cloud Debugger for .NET Core
.NET Core Debugger for the [Stackdriver Debugger](https://cloud.google.com/debugger/).

Stackdriver Debugger is a feature of the Google Cloud Platform that lets you inspect the state
of an application at any code location without using logging statements and without stopping or
slowing down your applications. 

## Overview
The .NET Core Stackdriver Debugger is comprised of four major parts

* **Debuggee** - The _Debuggee_ is the.NET Core application being debugged.
* **Stackdriver Debugger** - The _Stackdriver Debugger_ is a web application that allows
users to view source code and set snapshots on _Debuggees_.
* **.NET Debugger** - The _.NET Debugger_ is an implementation of the 
[`ICorDebug`](https://docs.microsoft.com/en-us/dotnet/framework/unmanaged-api/debugging/icordebug-interface)
interface used to debug the _Debugee_.
* **Debugging Agent** - The _Debugging Agent_ will run alongside the _.NET Debugger_.  It is responsible for
starting the _.NET Debugger_ as well as maintaining communication with the _Stackdriver Debugger_.

## Building
TODO(talarico): Fill out.

## Running
TODO(talarico): Fill out.

## Running Tests
TODO(talarico): Fill out.