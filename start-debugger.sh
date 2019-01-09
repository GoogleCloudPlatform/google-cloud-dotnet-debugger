#!/bin/bash
#
# Script to help start the users application along with
# the .NET Core Stackdriver Debugger.
#
# Input should be the command to start the users .NET Core
# application as they normally would.  For example if
# they would call 'dotnet HelloWorld.dll' they would
# just run ''./start-debugger.sh dotnet HelloWorld.dll'

start_command=
while (( "$#" )); do
  start_command="$start_command $1"
  shift
done

# Start the users application to avoid any slow down 
# while starting the debugger.
$start_command &

# Start the debugger and attach to the users process.
APP_PID=$(echo $!)
# NOTE: This file path is based on logic in package.sh
/usr/share/dotnet-debugger/agent/Google.Cloud.Diagnostics.Debug --application-id=$APP_PID &

# Wait on the users process.
wait $APP_PID
