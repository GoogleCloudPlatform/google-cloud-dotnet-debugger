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
    public class RequestLatencyTests : DebuggerTestBase
    {
        /// <summary>
        /// The average acceptable increase in latency when the debugger is attached.
        /// </summary>
        public const int AddedLatencyWhenDebuggingMs = 10;

        public RequestLatencyTests() : base() { }

        /// <summary>
        /// This test ensures the debugger does not add more than 10ms of
        /// latency to a request when the debugger is attached and no
        /// breakpoint is set.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_NoBreakpointsSet() => 
            await RunLatencyTestAsync();

        /// <summary>
        /// This test ensures the debugger does not add more than 10ms of
        /// latency to a request when the debugger is attached and
        /// breakpoint is set (but not hit).
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointSet() =>
            await RunLatencyTestAsync(breapointLine: TestApplication.HelloLine);

        /// <summary>
        /// This test ensures the debugger does not add more than 10ms of
        /// latency to a request when the debugger is attached and
        /// breakpoint is set (but not hit) with a condition that never
        /// evaluates to true in a tight loop.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointsSet_TightLoop() => await RunLatencyTestAsync(
            breapointLine: TestApplication.LoopMiddle, getUrl: TestApplication.GetLoopUrl, condition: "i == 2000");

        /// <summary>
        /// This test ensures the debugger does not add more than 10ms of
        /// latency to a request when the debugger is attached and
        /// breakpoint is hit.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointHit() =>
            await RunLatencyTestAsync(breapointLine: TestApplication.EchoTopLine, hitBreakpoint: true);

        /// <summary>
        /// Run a test to check latency while the debugger is enabled.
        /// This is tested by taking the average latency during requests to an
        /// application with no debugger attached and then the average latency during
        /// requests to the same application with a debugger attached (with the options
        /// breakpoints being set and hit during the requests).
        /// </summary>
        /// <param name="breapointLine">Optional, the line number to set the breakpoint on.  If none is set no
        ///     breakpoint will be set.</param>
        /// <param name="hitBreakpoint">Optional, true if the breakpoint is expected to hit.  Defaults to false.</param>
        /// <param name="condition">Optional, a condition to set on the breakpoint.  If none is set 
        ///     no condition will be set.</param>
        /// <param name="getUrl">Optional, a function to get the url to hit. Defaults to 
        ///     <see cref="TestApplication.GetEchoUrl(TestApplication, int)"/></param>
        private async Task RunLatencyTestAsync(int? breapointLine = null, bool hitBreakpoint = false,
             string condition = null, Func<TestApplication, int, string> getUrl = null)
        {
           double noDebugAvgLatency = await GetAverageLatencyAsync(debugEnabled: false);
           double debugAvgLatency = await GetAverageLatencyAsync(debugEnabled: true,
               breakpointLine: breapointLine, hitBreakpoint: hitBreakpoint, getUrl: getUrl, condition: condition);

            Console.WriteLine($"Average latency (ms) used w/o a debugger attached: {noDebugAvgLatency}");
            Console.WriteLine($"Average latency (ms) used w/ a debugger attached: {debugAvgLatency}");
            Console.WriteLine($"Latency increase (ms): {debugAvgLatency - noDebugAvgLatency}");

            Assert.True(debugAvgLatency <= noDebugAvgLatency + AddedLatencyWhenDebuggingMs,
              $"Avg latency (ms) w/o a debugger attached: {noDebugAvgLatency}\n" +
              $"Avg latency (ms) w/ a debugger attached: {debugAvgLatency}\n" +
              $"This is {debugAvgLatency - noDebugAvgLatency - AddedLatencyWhenDebuggingMs} more than expectable.");
        }

        /// <summary>
        /// Starts the test application (Google.Cloud.Diagnostics.Debug.TestApp) and
        /// gets the average latency for requests to the <see cref="AppUrlEcho"/> url for 
        /// <see cref="NumberOfRequest"/> requests.
        /// </summary>
        /// <param name="debugEnabled">True if the debugger should be attached to the application.</param>
        /// <param name="breapointLine">Optional, the line number to set the breakpoint on.  If none is set no
        ///     breakpoint will be set.</param>
        /// <param name="hitBreakpoint">Optional, true if the breakpoint is expected to hit.  Defaults to false.</param>
        /// <param name="condition">Optional, a condition to set on the breakpoint.  If none is set 
        ///     no condition will be set.</param>
        /// <param name="getUrl">Optional, a function to get the url to hit. Defaults to 
        ///     <see cref="TestApplication.GetEchoUrl(TestApplication, int)"/></param>
        /// <returns>The average latency of requests to the url.</returns>
        private async Task<double> GetAverageLatencyAsync(
            bool debugEnabled, int? breakpointLine = null, bool hitBreakpoint = false,
            string condition = null, Func<TestApplication, int, string> getUrl = null)
        {
            using (var app = StartTestApp(debugEnabled: debugEnabled))
            {
                var debuggee = debugEnabled ? Polling.GetDebuggee(app.Module, app.Version) : null;
                using (HttpClient client = new HttpClient())
                {
                    TimeSpan totalTime = TimeSpan.Zero;
                    for (int i = 0; i < NumberOfRequest; i++)
                    {
                        Debugger.V2.Breakpoint breakpoint = null;
                        if (breakpointLine != null)
                        {
                            // Set a breakpoint and wait to ensure the debuggee picks it up.
                            breakpoint = SetBreakpointAndSleep(
                                debuggee.Id, TestApplication.MainClass, breakpointLine.Value, condition);
                            Thread.Sleep(TimeSpan.FromSeconds(.5));
                        }

                        Stopwatch watch = Stopwatch.StartNew();
                        await client.GetAsync($"{app.AppUrlEcho}/{i}");
                        totalTime += watch.Elapsed;

                        if (breakpointLine != null)
                        {
                            var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id, isFinal: hitBreakpoint);
                            Assert.Equal(hitBreakpoint, newBp.IsFinalState);
                        }
                    }
                    return totalTime.TotalMilliseconds / NumberOfRequest;
                }
            }
        }
    }
}
