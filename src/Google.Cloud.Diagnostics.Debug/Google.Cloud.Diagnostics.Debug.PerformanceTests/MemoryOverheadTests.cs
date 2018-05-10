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

using Google.Cloud.Diagnostics.Debug.IntegrationTests;
using System;
using System.Diagnostics;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.PerformanceTests
{
    public class MemoryOverheadTests : DebuggerTestBase
    {
        /// <summary>
        /// The acceptable increased memory (10MB) when the debugger is attached.
        /// </summary>
        public const int AddedMemoryMB = 10;

        public MemoryOverheadTests() : base() { }

        /// <summary>
        /// This test ensures the debugger does not add more than 10MB of
        /// memory when the debugger is attached and no breakpoint is set.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_NoBreakpointsSet() =>
            await RunMemoryTestAsync();


        /// <summary>
        /// This test ensures the debugger does not add more than 10MB of
        /// memory when the debugger is attached and a breakpoint is set
        /// (but not hit).
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointsSet() =>
            await RunMemoryTestAsync(setBreakpoint: true);

        /// <summary>
        /// This test ensures the debugger does not add more than 10MB of
        /// memory when the debugger is attached and a breakpoint is hit.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointsHit() =>
             await RunMemoryTestAsync(setBreakpoint: true, hitBreakpoint: true);

        /// <summary>
        /// Run a test to check memory usage while the debugger is enabled.
        /// This is tested by taking the average memory during requests to an
        /// application with no debugger attached and then the average memory during
        /// requests to the same application with a debugger attached (with the options
        /// breakpoints being set and hit during the requests).
        /// </summary>
        /// <param name="setBreakpoint">Optional, true if a breakpoint should be set for the requests.
        ///     This is only during the debug enabled portion of the test. Defaults to false.</param>
        /// <param name="setBreakpoint">Optional, true if a breakpoint should be hit each request.
        ///     This is only during the debug enabled portion of the test. Defaults to false.</param>
        private async Task RunMemoryTestAsync(bool setBreakpoint = false, bool hitBreakpoint = false)
        {
            double noDebugAvgMemoryMB = await GetAverageMemoryUsageMBAsync(debugEnabled: false);
            double debugAvgMemoryMB = await GetAverageMemoryUsageMBAsync(debugEnabled: true,
                setBreakpoint: setBreakpoint, hitBreakpoint: hitBreakpoint);

            Console.WriteLine($"Average memory (in bytes) used w/o a debugger attached: {noDebugAvgMemoryMB}");
            Console.WriteLine($"Average memory (in bytes) used w/ a debugger attached: {debugAvgMemoryMB}");
            Console.WriteLine($"Memory increase (in bytes): {debugAvgMemoryMB - noDebugAvgMemoryMB}");

            Assert.True(debugAvgMemoryMB <= noDebugAvgMemoryMB + AddedMemoryMB,
               $"Average memory (in bytes) used w/o a debugger attached: {noDebugAvgMemoryMB}\n" +
               $"Average memory (in bytes) used w/ a debugger attached: {debugAvgMemoryMB}\n" +
               $"This is {debugAvgMemoryMB - noDebugAvgMemoryMB - AddedMemoryMB} more than expectable.");
        }

        /// <summary>
        /// Starts the test application (Google.Cloud.Diagnostics.Debug.TestApp) and
        /// gets the average memory usage during requests to <see cref="AppUrlEcho"/> url for 
        /// <see cref="NumberOfRequest"/> requests.
        /// </summary>
        /// <param name="debugEnabled">True if the debugger should be attached to the application.</param>
        /// <param name="setBreakpoint">Optional, true if a breakpoint should be set for the requests.
        ///     Defaults to false.</param>
        /// <param name="setBreakpoint">Optional, true if a breakpoint should be hit each request.
        ///     Defaults to false.</param>
        /// <returns>The average memory usage during requests.</returns>
        public async Task<double> GetAverageMemoryUsageMBAsync(
            bool debugEnabled, bool setBreakpoint = false, bool hitBreakpoint = false)
        {
            using (var app = StartTestApp(debugEnabled: debugEnabled))
            {
                var appProcess = await app.GetApplicationProcess();
                var debugProcess = app.GetDebuggerProcess();
                var agentProcess = app.GetAgentProcess();

                var debuggee = debugEnabled ? Polling.GetDebuggee(app.Module, app.Version) : null;

                int counter = 0;
                long memory = 0;
                var cts = new CancellationTokenSource();
                Task.Run(() =>
                {
                    while (!cts.IsCancellationRequested)
                    {
                        memory += appProcess.WorkingSet64;
                        if (debugEnabled)
                        {
                            memory += debugProcess.WorkingSet64;
                            memory += agentProcess.WorkingSet64;
                        }
                        counter++;
                        Thread.Sleep(TimeSpan.FromMilliseconds(2));
                    }
                });

                using (HttpClient client = new HttpClient())
                {
                   for (int i = 0; i < NumberOfRequest; i++)
                    {
                        Debugger.V2.Breakpoint breakpoint = null;
                        if (setBreakpoint)
                        {
                            var line = hitBreakpoint ? TestApplication.EchoTopLine : TestApplication.HelloLine;
                            // Set a breakpoint and wait to ensure the debuggee picks it up.
                            breakpoint = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, line);
                            Thread.Sleep(TimeSpan.FromSeconds(.5));
                        }

                       await client.GetAsync($"{app.AppUrlEcho}/{i}");

                        if (setBreakpoint)
                        {
                            var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id, isFinal: hitBreakpoint);
                            Assert.Equal(hitBreakpoint, newBp.IsFinalState);
                        }
                    }
                    cts.Cancel();
                    return (memory /counter / NumberOfRequest) / Math.Pow(2, 20);
                }
            }
        }
    }
}
