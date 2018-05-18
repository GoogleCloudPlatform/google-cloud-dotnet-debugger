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
        public async Task DebuggerAttached_BreakpointsSet() => await RunCpuTestAsync(
            AddedCpuWhenDebuggingPercent, breapointLine: TestApplication.HelloLine);

        /// <summary>
        /// This test ensures the debugger does not add more than 0.1% of
        /// CPU time to a when the debugger is attached and
        /// breakpoint is set (but not hit) with a condition that never
        /// evaluates to true in a tight loop.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointsSet_TightLoop() => await RunCpuTestAsync(
           AddedCpuWhenDebuggingPercent, breapointLine: TestApplication.LoopMiddle,
           hitUrl: TestApplication.GetLoopUrl, condition: "i == 2000");

        /// <summary>
        /// This test ensures the debugger does not add more than 1% of
        /// CPU time to a when the debugger is attached and
        /// breakpoint is hit.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointHit() =>  await RunCpuTestAsync(
                AddedCpuWhenEvaluatingPercent, breapointLine: TestApplication.EchoTopLine, hitBreakpoint: true);

        /// <summary>
        /// Run a test to check CPU usage while the debugger is enabled.
        /// This is tested by taking the CPU usage during requests to an
        /// application with no debugger attached and then the CPU usage during
        /// requests to the same application with a debugger attached (with the options
        /// breakpoints being set and hit during the requests).
        /// </summary>
        /// <param name="breapointLine">Optional, the line number to set the breakpoint on.  If none is set no
        ///     breakpoint will be set.</param>
        /// <param name="hitBreakpoint">Optional, true if the breakpoint is expected to hit.  Defaults to false.</param>
        /// <param name="condition">Optional, a condition to set on the breakpoint.  If none is set 
        ///     no condition will be set.</param>
        /// <param name="hitUrl">Optional, a function to get the url to hit. Defaults to 
        ///     <see cref="TestApplication.GetEchoUrl(TestApplication, int)"/></param>
        private async Task RunCpuTestAsync(
            double acceptableCpuIncrease, int? breapointLine = null, bool hitBreakpoint = false,
             string condition = null, Func<TestApplication, int, string> hitUrl = null)
        {
            double noDebugAvgPercentCpu = await GetAverageCpuPercentAsync(debugEnabled: false);
            double debugAvgPercentCpu = await GetAverageCpuPercentAsync(debugEnabled: true,
                breakpointLine: breapointLine, hitBreakpoint: hitBreakpoint, hitUrl: hitUrl, condition: condition);

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
        /// <param name="breapointLine">Optional, the line number to set the breakpoint on.  If none is set no
        ///     breakpoint will be set.</param>
        /// <param name="hitBreakpoint">Optional, true if the breakpoint is expected to hit.  Defaults to false.</param>
        /// <param name="condition">Optional, a condition to set on the breakpoint.  If none is set 
        ///     no condition will be set.</param>
        /// <param name="hitUrl">Optional, a function to get the url to hit. Defaults to 
        ///     <see cref="TestApplication.GetEchoUrl(TestApplication, int)"/></param>
        /// <returns>The average CPU percentage during requests.</returns>
        private async Task<double> GetAverageCpuPercentAsync(
            bool debugEnabled, int? breakpointLine = null, bool hitBreakpoint = false,
            string condition = null, Func<TestApplication, int, string> hitUrl = null)
        {
            hitUrl = hitUrl ?? TestApplication.GetEchoUrl;

            using (var app = StartTestApp(debugEnabled: debugEnabled))
            {
                var appProcess = await app.GetApplicationProcess();
                var debugProcess = app.GetDebuggerProcess();
                var agentProcess = app.GetAgentProcess();

                Stopwatch watch = Stopwatch.StartNew();
                var startingAppCpu = appProcess.TotalProcessorTime;
                var startingDebugCpu = TimeSpan.Zero;
                var startingAgentCpu = TimeSpan.Zero;
                if (debugEnabled)
                {
                    startingDebugCpu = debugProcess.TotalProcessorTime;
                    startingAgentCpu = agentProcess.TotalProcessorTime;
                }

                var debuggee = debugEnabled ? Polling.GetDebuggee(app.Module, app.Version) : null;

                using (HttpClient client = new HttpClient())
                {
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

                        await client.GetAsync(hitUrl(app, i));

                        if (breakpointLine != null)
                        {
                            var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id, isFinal: hitBreakpoint);
                            Assert.Equal(hitBreakpoint, newBp.IsFinalState);
                        }
                    }
                    var totalCpuTime = appProcess.TotalProcessorTime - startingAppCpu;
                    if (debugEnabled)
                    {
                        totalCpuTime += debugProcess.TotalProcessorTime - startingDebugCpu;
                        totalCpuTime += agentProcess.TotalProcessorTime - startingAgentCpu;
                    }
                    return totalCpuTime.TotalMilliseconds / watch.ElapsedMilliseconds;
                }
            }
        }
    }
}
