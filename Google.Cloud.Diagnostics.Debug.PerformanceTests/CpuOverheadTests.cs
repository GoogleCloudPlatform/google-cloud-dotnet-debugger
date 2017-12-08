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
    public class CpuOverheadTests : DebuggerTestBase
    {
        /// <summary>
        /// The average acceptable percent increase in CPU when the debugger is attached.
        /// </summary>
        public const double AddedCpuWhenDebuggingPercent = 0.001;

        /// <summary>
        /// The average acceptable percent increase in CPU when the debugger is attached and debugging.
        /// </summary>
        public const double AddedCpuWhenEvaluatingPercent = 0.01;

        public CpuOverheadTests() : base() { }

        /// <summary>
        /// This test ensures the debugger does not add more than 0.1% of
        /// CPU time to a when the debugger is attached and no
        /// breakpoint is set
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_NoBreakpointsSet() =>
            await RunCpuTestAsync(AddedCpuWhenDebuggingPercent);

        /// <summary>
        /// This test ensures the debugger does not add more than 0.1% of
        /// CPU time to a when the debugger is attached and
        /// breakpoint is set (but not hit).
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointsSet() =>
            await RunCpuTestAsync(AddedCpuWhenEvaluatingPercent, setBreakpoint: true);

        /// <summary>
        /// This test ensures the debugger does not add more than 1% of
        /// CPU time to a when the debugger is attached and
        /// breakpoint is hit.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointHit() => 
            await RunCpuTestAsync(
                AddedCpuWhenDebuggingPercent, setBreakpoint: true, hitBreakpoint: true);

        /// <summary>
        /// Run a test to check CPU usage while the debugger is enabled.
        /// This is tested by taking the CPU usage during requests to an
        /// application with no debugger attached and then the CPU usage during
        /// requests to the same application with a debugger attached (with the options
        /// breakpoints being set and hit during the requests).
        /// </summary>
        /// <param name="setBreakpoint">Optional, true if a breakpoint should be set for the requests.
        ///     This is only during the debug enabled portion of the test. Defaults to false.</param>
        /// <param name="setBreakpoint">Optional, true if a breakpoint should be hit each request.
        ///     This is only during the debug enabled portion of the test. Defaults to false.</param>
        private async Task RunCpuTestAsync(double acceptableCpuIncrease, bool setBreakpoint = false, bool hitBreakpoint = false)
        {
            double noDebugAvgPercentCpu = await GetAverageCpuPercentAsync(debugEnabled: false);
            double debugAvgPercentCpu = await GetAverageCpuPercentAsync(debugEnabled: true,
                setBreakpoint: setBreakpoint, hitBreakpoint: hitBreakpoint);

            Console.WriteLine($"Percent CPU time w/o a debugger attached: {noDebugAvgPercentCpu}");
            Console.WriteLine($"Percent CPU time w/ a debugger attached: {debugAvgPercentCpu}");
            Console.WriteLine($"Percent CPU increase: {debugAvgPercentCpu - noDebugAvgPercentCpu}");

            Assert.True(debugAvgPercentCpu <= noDebugAvgPercentCpu + acceptableCpuIncrease,
               $"Percent CPU time w/o a debugger attached: {noDebugAvgPercentCpu}\n" +
               $"Percent CPU time w/ a debugger attached: {debugAvgPercentCpu}\n" +
               $"This is {debugAvgPercentCpu - noDebugAvgPercentCpu - acceptableCpuIncrease} more than expectable.");
        }

        /// <summary>
        /// Starts the test application (Google.Cloud.Diagnostics.Debug.TestApp) and
        /// gets the average CPU percentage during requests to <see cref="AppUrlEcho"/> url for 
        /// <see cref="NumberOfRequest"/> requests.
        /// </summary>
        /// <param name="debugEnabled">True if the debugger should be attached to the application.</param>
        /// <param name="setBreakpoint">Optional, true if a breakpoint should be set for the requests.
        ///     Defaults to false.</param>
        /// <param name="setBreakpoint">Optional, true if a breakpoint should be hit each request.
        ///     Defaults to false.</param>
        /// <returns>The average CPU percentage during requests.</returns>
        private async Task<double> GetAverageCpuPercentAsync(
            bool debugEnabled, bool setBreakpoint = false, bool hitBreakpoint = false)
        {
            using (StartTestApp(debugEnabled: debugEnabled))
            {
                var processId = await GetProcessId();
                var process = Process.GetProcessById(processId);
                var debuggee = debugEnabled ? Polling.GetDebuggee(Module, Version) : null;

                using (HttpClient client = new HttpClient())
                {
                    TimeSpan totalTime = TimeSpan.Zero;
                    TimeSpan totalCpuTime = TimeSpan.Zero;
                    for (int i = 0; i < NumberOfRequest; i++)
                    {
                        Debugger.V2.Breakpoint breakpoint = null;
                        if (setBreakpoint)
                        {
                            var line = hitBreakpoint ? 32 : 26;
                            // Set a breakpoint and wait to ensure the debuggee picks it up.
                            breakpoint = SetBreakpoint(debuggee.Id, "MainController.cs", line);
                            Thread.Sleep(TimeSpan.FromSeconds(.5));
                        }

                        Stopwatch watch = Stopwatch.StartNew();
                        var startingCpu = process.TotalProcessorTime;
                        HttpResponseMessage result = await client.GetAsync($"{AppUrlEcho}/{i}");
                        totalCpuTime += process.TotalProcessorTime - startingCpu;
                        totalTime += watch.Elapsed;

                        if (setBreakpoint)
                        {
                            var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id, isFinal: hitBreakpoint);
                            Assert.Equal(hitBreakpoint, newBp.IsFinalState);
                        }
                    }
                    return totalCpuTime.TotalMilliseconds / totalTime.TotalMilliseconds;
                }
            }
        }
    }
}
