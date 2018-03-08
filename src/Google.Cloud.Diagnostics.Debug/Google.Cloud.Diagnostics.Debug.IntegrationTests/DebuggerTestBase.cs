// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

using Google.Cloud.Debugger.V2;
using System;
using System.Net.Http;
using System.Threading;

namespace Google.Cloud.Diagnostics.Debug.IntegrationTests
{
    /// <summary>
    /// A base class for debugger integration and performance tests to
    /// reduce boiler plate code.
    /// </summary>
    public class DebuggerTestBase
    {
        /// <summary>The number of requests to test against. </summary>
        public readonly int NumberOfRequest = 100;

        /// <summary>A helper to get elements from the debugger api.</summary>
        public readonly DebuggerPolling Polling;

        public DebuggerTestBase()
        {
            Polling = new DebuggerPolling();
        }

        /// <summary>
        /// Set a breakpoint at a file and line for a given debuggee and then waits 1 second.
        /// The wait will ensure the breakpoint is picked up by the agent.
        /// </summary>
        public Debugger.V2.Breakpoint SetBreakpointAndSleep(string debuggeeId, string path, int line)
        {
            var breakpoint = SetBreakpoint(debuggeeId, path, line);
            Thread.Sleep(TimeSpan.FromSeconds(1));
            return breakpoint;
        }

        /// <summary>
        /// Set a breakpoint at a file and line for a given debuggee.
        /// </summary>
        public Debugger.V2.Breakpoint SetBreakpoint(string debuggeeId, string path, int line)
        {
            SetBreakpointRequest request = new SetBreakpointRequest
            {
                DebuggeeId = debuggeeId,
                Breakpoint = new Debugger.V2.Breakpoint
                {
                    Location = new Debugger.V2.SourceLocation
                    {
                        Path = path,
                        Line = line,
                    }
                }
            };
            return Polling.Client.GrpcClient.SetBreakpoint(request).Breakpoint;
        }

        /// <summary>
        /// Start the test application.
        /// </summary>
        /// <param name="debugEnabled">True if the debugger should be started with and attached
        ///     to the app</param>
        /// <param name="waitForStart">Optional. True if this method should block until the
        ///     application is started and can be queried.  Defaults to true.</param>
        /// <returns>A test application.</returns>
        public TestApplication StartTestApp(bool debugEnabled, bool waitForStart = true)
        { 
            var app = new TestApplication(debugEnabled);

            if (waitForStart)
            {
                using (HttpClient client = new HttpClient())
                {
                    // Allow the app a chance to start up as it may not start
                    // right away.
                    int attempts = 10;
                    for (int i = 0; i < attempts; i++)
                    {
                        try
                        {
                            client.GetAsync(app.AppUrlBase).Wait();
                            break;
                        }
                        catch (AggregateException) when (i < attempts - 1)
                        {
                            Thread.Sleep(TimeSpan.FromSeconds(1));
                        }
                    }
                }
            }
            return app;
        }
    }
}
