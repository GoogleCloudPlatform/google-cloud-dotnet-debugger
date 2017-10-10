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
using System.Threading.Tasks;
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.PerformanceTests
{
    public class StartupTests : DebuggerTestBase
    {
        /// <summary>
        /// The average acceptable increase in latency when the debugger is starting
        /// the users application.
        /// </summary>
        public const int AddedStartTimeWhenDebuggingMs = 10;

        /// <summary>
        /// The number of requests to test against. 
        /// </summary>
        public const int NumberOfRequest = 10;

        /// <summary>
        /// This tests ensure that the start up time of the application is not affected
        /// by the debugger.
        /// 
        /// This is tested by starting the application and trying to query a url and taking
        /// the time it takes to get a response from the application.
        /// </summary>
        /// <returns></returns>
        [Fact]
        public async Task DebuggerAttached()
        {
            Func<bool, Task<double>> averageStartTimeMs = async delegate (bool debugEnabled)
            {
                double totalStartTimeMs = 0;
                for (int i = 0; i < NumberOfRequest; i++)
                {
                    using (HttpClient client = new HttpClient())
                    {
                        Stopwatch watch = Stopwatch.StartNew();
                        using (StartTestApp(debugEnabled: debugEnabled))
                        {
                            // Allow for retries, this may happen if the app is
                            // taking a very long time to start.
                            for (int j = 0; j < 5; j++)
                            {
                                try
                                {
                                    await client.GetAsync($"{AppUrlEcho}/{i}");
                                    break;
                                }
                                catch (HttpRequestException) { }
                            }
                        }
                        totalStartTimeMs += watch.Elapsed.TotalMilliseconds;
                    }
                }
                return totalStartTimeMs / NumberOfRequest;
            };

            var noDebugAvgStartTimeMs = await averageStartTimeMs(false);
            var debugAvgStartTimeMs = await averageStartTimeMs(true);

            Assert.True(debugAvgStartTimeMs <= noDebugAvgStartTimeMs + AddedStartTimeWhenDebuggingMs,
              $"Avg start time w/o a debugger attached: {noDebugAvgStartTimeMs}\n" +
              $"Avg start time w/ a debugger attached: {debugAvgStartTimeMs}\n" +
              $"This is {debugAvgStartTimeMs - noDebugAvgStartTimeMs - AddedStartTimeWhenDebuggingMs} more than expectable.");
        }
    }
}
