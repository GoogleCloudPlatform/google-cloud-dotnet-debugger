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

        /// <summary>
        /// The number of requests to test against. 
        /// </summary>
        public const int NumberOfRequest = 100;

        public RequestLatencyTests() : base() { }

        /// <summary>
        /// This test ensures the debugger does not add more than 10ms of
        /// latency to a request when the debugger is attached and no
        /// breakpoint is set.
        /// 
        /// This is tested by taking the average latency of request to an
        /// application with no debugger attached and then the average latency
        /// of requests to the same application with a debugger attached.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_NoBreakpointsSet()
        {
            double noDebugAvgLatency;
            using (StartTestApp(debugEnabled: false))
            {
                noDebugAvgLatency = await GetAverageLatencyAsync(NumberOfRequest);
            }

            double debugAvgLatency;
            using (StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(Module, Version);
                debugAvgLatency = await GetAverageLatencyAsync(NumberOfRequest);
            }

            AssertAcceptableLatency(noDebugAvgLatency, debugAvgLatency);
        }

        /// <summary>
        /// This test ensures the debugger does not add more than 10ms of
        /// latency to a request when the debugger is attached and
        /// breakpoint is set (but not hit).
        /// 
        /// This is tested by taking the average latency of request to an
        /// application with no debugger attached and then the average latency
        /// of requests to the same application with a debugger attached.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointSet()
        {
            double noDebugAvgLatency;
            using (StartTestApp(debugEnabled: false))
            {
                noDebugAvgLatency = await GetAverageLatencyAsync(NumberOfRequest);
            }

            double debugAvgLatency;
            using (StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(Module, Version);
                var breakpoint = SetBreakpoint(debuggee.Id, "MainController.cs", 25);

                debugAvgLatency = await GetAverageLatencyAsync(NumberOfRequest);

                var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id, isFinal: false);
                Assert.False(newBp.IsFinalState);
            }

            AssertAcceptableLatency(noDebugAvgLatency, debugAvgLatency);
        }


        /// <summary>
        /// This test ensures the debugger does not add more than 10ms of
        /// latency to a request when the debugger is attached and
        /// breakpoint is hit.
        /// 
        /// This is tested by taking the average latency of request to an
        /// application with no debugger attached and then the average latency
        /// of requests to the same application with a debugger attached.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointHit()
        {
            double noDebugAvgLatency;
            using (StartTestApp(debugEnabled: false))
            {
                noDebugAvgLatency = await GetAverageLatencyAsync(NumberOfRequest);
            }

            double debugAvgLatency;
            using (StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(Module, Version);

                using (HttpClient client = new HttpClient())
                {
                    TimeSpan totalTime = TimeSpan.Zero;
                    for (int i = 0; i < NumberOfRequest; i++)
                    {
                        // Set a breakpoint and wait to ensure the debuggee picks it up.
                        var breakpoint = SetBreakpoint(debuggee.Id, "MainController.cs", 31);
                        Thread.Sleep(TimeSpan.FromSeconds(.5));

                        Stopwatch watch = Stopwatch.StartNew();
                        await client.GetAsync($"{AppUrlEcho}/{i}");
                        totalTime += watch.Elapsed;
                       
                        var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);
                        Assert.True(newBp.IsFinalState);
                    }
                    debugAvgLatency = totalTime.TotalMilliseconds / NumberOfRequest;
                }                
            }
            AssertAcceptableLatency(noDebugAvgLatency, debugAvgLatency);
        }

        /// <summary>
        /// Assert that the latency of requests to an app with a debugger and without are within
        /// the acceptable range (10ms).
        /// </summary>
        /// <param name="noDebugAvgLatency">The average latency for requests to an app with no debugger attached.</param>
        /// <param name="debugAvgLatency">The average latency for requests to an app with a debugger attached.</param>
        private void AssertAcceptableLatency(double noDebugAvgLatency, double debugAvgLatency)
        {
            Assert.True(debugAvgLatency <= noDebugAvgLatency + AddedLatencyWhenDebuggingMs,
               $"Avg latency w/o a debugger attached: {noDebugAvgLatency}\n" +
               $"Avg latency w/ a debugger attached: {debugAvgLatency}\n" +
               $"This is {debugAvgLatency - noDebugAvgLatency - AddedLatencyWhenDebuggingMs} more than expectable.");
        }
    }
}
