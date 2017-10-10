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
        /// The number of requests to test against. 
        /// </summary>
        public const int NumberOfRequest = 100;

        /// <summary>
        /// The acceptable increased memory (10MB) when the debugger is attached.
        /// </summary>
        public const int AddedMemoryMB = 10;

        public MemoryOverheadTests() : base() { }

        /// <summary>
        /// This test ensures the debugger does not add more than 10MB of
        /// memory when the debugger is attached and no breakpoint is set.
        /// 
        /// This is tested by taking the average memory during requests to an
        /// application with no debugger attached and then the average memory during
        /// requests to the same application with a debugger attached.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_NoBreakpointsSet()
        {
            double noDebugAvgMemoryMB;
            using (StartTestApp(debugEnabled: false))
            {
                noDebugAvgMemoryMB = await GetAverageMemoryUsageMBAsync(NumberOfRequest);
            }

            double debugAvgMemoryMB;
            using (StartTestApp(debugEnabled: true))
            {
                debugAvgMemoryMB = await GetAverageMemoryUsageMBAsync(NumberOfRequest);
            }

            AssertAcceptableMemory(noDebugAvgMemoryMB, debugAvgMemoryMB);
        }

        /// <summary>
        /// This test ensures the debugger does not add more than 10MB of
        /// memory when the debugger is attached and a breakpoint is set
        /// (but not hit).
        /// 
        /// This is tested by taking the average memory during requests to an
        /// application with no debugger attached and then the average memory during
        /// requests to the same application with a debugger attached.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointsSet()
        {
            double noDebugAvgMemoryMB;
            using (StartTestApp(debugEnabled: false))
            {
                noDebugAvgMemoryMB = await GetAverageMemoryUsageMBAsync(NumberOfRequest);
            }

            double debugAvgMemoryMB;
            using (StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(Module, Version);
                var breakpoint = SetBreakpoint(debuggee.Id, "MainController.cs", 25);

                debugAvgMemoryMB = await GetAverageMemoryUsageMBAsync(NumberOfRequest);

                var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id, isFinal: false);
                Assert.False(newBp.IsFinalState);
            }

            AssertAcceptableMemory(noDebugAvgMemoryMB, debugAvgMemoryMB);
        }


        /// <summary>
        /// This test ensures the debugger does not add more than 10MB of
        /// memory when the debugger is attached and a breakpoint is hit.
        /// 
        /// This is tested by taking the average memory during requests to an
        /// application with no debugger attached and then the average memory during
        /// requests to the same application with a debugger attached.
        /// </summary>
        [Fact]
        public async Task DebuggerAttached_BreakpointsHit()
        {
            double noDebugAvgMemoryMB;
            using (StartTestApp(debugEnabled: false))
            {
                noDebugAvgMemoryMB = await GetAverageMemoryUsageMBAsync(NumberOfRequest);
            }

            double debugAvgMemoryMB;
            using (StartTestApp(debugEnabled: true))
            {
                var processId = await GetProcessId();
                var process = Process.GetProcessById(processId);
                var debuggee = Polling.GetDebuggee(Module, Version);

                long totalMemory = 0;
                using (HttpClient client = new HttpClient())
                {
                    for (int i = 0; i < NumberOfRequest; i++)
                    {
                        // Set a breakpoint and wait to ensure the debuggee picks it up.
                        var breakpoint = SetBreakpoint(debuggee.Id, "MainController.cs", 31);
                        Thread.Sleep(TimeSpan.FromSeconds(.5));

                        int counter = 0;
                        long memory = 0;
                        Task<HttpResponseMessage> task = client.GetAsync($"{AppUrlEcho}/{i}");
                        // TODO(talarico): Can we do better?
                        while (!task.IsCompleted)
                        {
                            memory += process.WorkingSet64;
                            counter++;
                            Timer.Sleep(TimeSpan.FromMilliseconds(2));
                        }
                        totalMemory += memory / counter;

                        var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);
                        Assert.True(newBp.IsFinalState);
                    }
                }
                debugAvgMemoryMB = (totalMemory / NumberOfRequest) / Math.Pow(2, 20);
            }

            AssertAcceptableMemory(noDebugAvgMemoryMB, debugAvgMemoryMB);
        }

        /// <summary>
        /// Assert that the average memory usage during requests to an app with a debugger and without are within
        /// the acceptable range.
        /// </summary>
        /// <param name="noDebugAvgMemoryMB">The average memory for an app with no debugger attached.</param>
        /// <param name="debugAvgMemoryMB">The average memory for an app with a debugger attached.</param>
        private void AssertAcceptableMemory(double noDebugAvgMemoryMB, double debugAvgMemoryMB)
        {
            Console.WriteLine($"Average memory (in bytes) used w/o a debugger attached: {noDebugAvgMemoryMB}");
            Console.WriteLine($"Average memory (in bytes) used w/ a debugger attached: {debugAvgMemoryMB}");
            Console.WriteLine($"Memory increase (in bytes): {debugAvgMemoryMB - noDebugAvgMemoryMB}");

            Assert.True(debugAvgMemoryMB <= noDebugAvgMemoryMB + AddedMemoryMB,
               $"Average memory (in bytes) used w/o a debugger attached: {noDebugAvgMemoryMB}\n" +
               $"Average memory (in bytes) used w/ a debugger attached: {debugAvgMemoryMB}\n" +
               $"This is {debugAvgMemoryMB - noDebugAvgMemoryMB - AddedMemoryMB} more than expectable.");
        }
    }
}
