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
        /// The number of requests to test against. 
        /// </summary>
        public const int NumberOfRequest = 100;

        /// <summary>
        /// The average acceptable percent increase in CPU when the debugger is attached.
        /// </summary>
        public const double AddedCpuWhenDebuggingPercent = 0.001;

        /// <summary>
        /// The average acceptable percent increase in CPU when the debugger is attached and debugging.
        /// </summary>
        public const double AddedCpuWhenEvaluatingPercent = 0.01;

        /// <summary>
        /// This test ensures the debugger does not add more than 0.1% of
        /// CPU time to a when the debugger is attached and no
        /// breakpoint is set
        /// 
        /// This is tested by taking the CPU time during requests to an
        /// application with no debugger attached and then the CPU time during
        /// of requests to the same application with a debugger attached.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_NoBreakpointsSet()
        {
            double noDebugAvgPercentCpu;
            using (StartTestApp(debugEnabled: false))
            {
                noDebugAvgPercentCpu = await GetAverageCpuPercentAsync(NumberOfRequest);
            }

            double debugAvgPercentCpu;
            using (StartTestApp(debugEnabled: true))
            {
                debugAvgPercentCpu = await GetAverageCpuPercentAsync(NumberOfRequest);
            }

            AssertAcceptableCpu(noDebugAvgPercentCpu, debugAvgPercentCpu, AddedCpuWhenDebuggingPercent);
        }

        /// <summary>
        /// This test ensures the debugger does not add more than 0.1% of
        /// CPU time to a when the debugger is attached and
        /// breakpoint is set (but not hit).
        /// 
        /// This is tested by taking the average CPU time during requests to an
        /// application with no debugger attached and then the CPU time during
        /// requests to the same application with a debugger attached.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointsSet()
        {
            double noDebugAvgPercentCpu;
            using (StartTestApp(debugEnabled: false))
            {
                noDebugAvgPercentCpu = await GetAverageCpuPercentAsync(NumberOfRequest);
            }

            double debugAvgPercentCpu;
            using (StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(Module, Version);
                var breakpoint = SetBreakpoint(debuggee.Id, "MainController.cs", 25);
                debugAvgPercentCpu = await GetAverageCpuPercentAsync(NumberOfRequest);
            }

            AssertAcceptableCpu(noDebugAvgPercentCpu, debugAvgPercentCpu, AddedCpuWhenDebuggingPercent);
        }

        /// <summary>
        /// This test ensures the debugger does not add more than 1% of
        /// CPU time to a when the debugger is attached and
        /// breakpoint is hit.
        /// 
        /// This is tested by taking the average lCPU time during requests to an
        /// application with no debugger attached and CPU time during
        /// of requests to the same application with a debugger attached.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointHit()
        {
            double noDebugAvgPercentCpu;
            using (StartTestApp(debugEnabled: false))
            {
                noDebugAvgPercentCpu = await GetAverageCpuPercentAsync(NumberOfRequest);
            }

            double debugAvgPercentCpu;
            using (StartTestApp(debugEnabled: true))
            {
                var processId = await GetProcessId();
                var debugProcess = Process.GetProcessById(processId);
                var debuggee = Polling.GetDebuggee(Module, Version);

                TimeSpan totalTime = TimeSpan.Zero;
                TimeSpan totalCpuTime = TimeSpan.Zero;
                using (HttpClient client = new HttpClient())
                {
                    for (int i = 0; i < NumberOfRequest; i++)
                    {
                        // Set a breakpoint and wait to ensure the debuggee picks it up.
                        var breakpoint = SetBreakpoint(debuggee.Id, "MainController.cs", 31);
                        Thread.Sleep(TimeSpan.FromSeconds(.5));

                        Stopwatch watch = Stopwatch.StartNew();
                        var startingCpu = debugProcess.TotalProcessorTime;
                        await client.GetAsync($"{AppUrlEcho}/{i}");
                        totalCpuTime += debugProcess.TotalProcessorTime - startingCpu;
                        totalTime += watch.Elapsed;

                        var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);
                        Assert.True(newBp.IsFinalState);
                    }
                }
                debugAvgPercentCpu = totalCpuTime.TotalMilliseconds / totalTime.TotalMilliseconds;
            }

            AssertAcceptableCpu(noDebugAvgPercentCpu, debugAvgPercentCpu, AddedCpuWhenEvaluatingPercent);
        }

        /// <summary>
        /// Assert that the CPU usage during requests to an app with a debugger and without are within
        /// the acceptable range.
        /// </summary>
        /// <param name="noDebugAvgPercentCpu">The average percentage of CPU time for an app with no debugger attached.</param>
        /// <param name="debugAvgPercentCpu">The average percentage of CPU time for an app with a debugger attached.</param>
        /// <param name="acceptableCpuIncrease">The acceptable percentage of CPU increase.</param>
        private void AssertAcceptableCpu(double noDebugAvgPercentCpu, double debugAvgPercentCpu, double acceptableCpuIncrease)
        {
            Console.WriteLine($"Percent CPU time w/o a debugger attached: {noDebugAvgPercentCpu}");
            Console.WriteLine($"Percent CPU time w/ a debugger attached: {debugAvgPercentCpu}");
            Console.WriteLine($"Percent CPU increase: {debugAvgPercentCpu - noDebugAvgPercentCpu}");

            Assert.True(debugAvgPercentCpu <= noDebugAvgPercentCpu + acceptableCpuIncrease,
               $"Percent CPU time w/o a debugger attached: {noDebugAvgPercentCpu}\n" +
               $"Percent CPU time w/ a debugger attached: {debugAvgPercentCpu}\n" +
               $"This is {debugAvgPercentCpu - noDebugAvgPercentCpu - acceptableCpuIncrease} more than expectable.");
        }
    }
}
