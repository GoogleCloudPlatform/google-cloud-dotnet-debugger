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

            Assert.True(debugAvgLatency <= noDebugAvgLatency + AddedLatencyWhenDebuggingMs);
        }

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

            Assert.True(debugAvgLatency <= noDebugAvgLatency + AddedLatencyWhenDebuggingMs);
        }
    }
}
