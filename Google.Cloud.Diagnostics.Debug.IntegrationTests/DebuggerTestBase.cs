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
using System.Diagnostics;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;

namespace Google.Cloud.Diagnostics.Debug.IntegrationTests
{
    /// <summary>
    /// A base class for debugger integration and performance tests to
    /// reduce boiler plate code.
    /// </summary>
    public class DebuggerTestBase
    {
        /// <summary>The base url for the test application.</summary>
        public static readonly string AppUrlBase = "http://localhost:5000";

        /// <summary>The url to forcibly shutdown the test application.</summary>
        public static readonly string AppUrlShutdown = $"{AppUrlBase}/Main/Shutdown";

        /// <summary>The url to echo the last piece of the path from the test application.</summary>
        public static readonly string AppUrlEcho = $"{AppUrlBase}/Main/Echo";

        /// <summary>The url to get the process id from the test application.</summary>
        public static readonly string AppUrlProcessId = $"{AppUrlBase}/Main/ProcessId";

        /// <summary>The module of the a debuggee.</summary>
        public readonly string Module;

        /// <summary>The version of the a debuggee.</summary>
        public readonly string Version;

        /// <summary>The Google CLoud Console project id to test with.</summary>
        public readonly string ProjectId;

        /// <summary>A default set of debugger options.</summary>
        public readonly AgentOptions Options;

        /// <summary>A helper to get elements from the debugger api.</summary>
        public readonly DebuggerPolling Polling;

        public DebuggerTestBase()
        {
            Module = nameof(DebuggerTestBase);
            Polling = new DebuggerPolling();
            Version = Guid.NewGuid().ToString();
            ProjectId = Utils.GetProjectIdFromEnvironment();
            Options = new AgentOptions
            {
                Module = Module,
                Version = Version,
                ProjectId = ProjectId,
                Debugger = Utils.GetDebugger(),
                Application = Utils.GetApplication(),
            };
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
        /// Gets the average latency for requests to the <see cref="AppUrlEcho"/> url.
        /// </summary>
        /// <param name="numRequests">The number of requests to sample.</param>
        /// <returns>The average latency of requests to the url.</returns>
        public async Task<double> GetAverageLatencyAsync(int numRequests)
        {
            using (HttpClient client = new HttpClient())
            {
                TimeSpan totalTime = TimeSpan.Zero;
                for (int i = 0; i < numRequests; i++)
                {
                    Stopwatch watch = Stopwatch.StartNew();
                    await client.GetAsync($"{AppUrlEcho}/{i}");
                    totalTime += watch.Elapsed;
                }
                return totalTime.TotalMilliseconds / numRequests;
            }
        }

        /// <summary>
        /// Gets the average CPU percentage during requests.
        /// </summary>
        /// <param name="numRequests">The number of requests to sample.</param>
        /// <returns>The average CPU percentage during requests.</returns>
        public async Task<double> GetAverageCpuPercentAsync(int numRequests)
        {
            var processId = await GetProcessId();
            var process = Process.GetProcessById(processId);

            using (HttpClient client = new HttpClient())
            {
                TimeSpan totalTime = TimeSpan.Zero;
                TimeSpan totalCpuTime = TimeSpan.Zero;
                for (int i = 0; i < numRequests; i++)
                {
                    Stopwatch watch = Stopwatch.StartNew();
                    var startingCpu = process.TotalProcessorTime;
                    HttpResponseMessage result = await client.GetAsync($"{AppUrlEcho}/{i}");
                    totalCpuTime += process.TotalProcessorTime - startingCpu;
                    totalTime += watch.Elapsed;
                }
                return totalCpuTime.TotalMilliseconds / totalTime.TotalMilliseconds;
            }
        }

        /// <summary>
        /// Gets the average memory usage during requests.
        /// </summary>
        /// <param name="numRequests">The number of requests to sample.</param>
        /// <returns>The average memory usage during requests.</returns>
        public async Task<double> GetAverageMemoryUsageMBAsync(int numRequests)
        {
            var processId = await GetProcessId();
            var process = Process.GetProcessById(processId);

            using (HttpClient client = new HttpClient())
            {
                long totalMemory = 0;
                for (int i = 0; i < numRequests; i++)
                {
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
                }
                return (totalMemory / numRequests) / Math.Pow(2, 20);
            }
        }

        /// <summary>
        /// Gets the process id for the running test application.
        /// </summary>
        public async Task<int> GetProcessId()
        {
            using (HttpClient client = new HttpClient())
            {
                HttpResponseMessage result = await client.GetAsync(AppUrlProcessId);
                var resultStr = await result.Content.ReadAsStringAsync();
                return Int32.Parse(resultStr);
            }
        }

        /// <summary>
        /// Start the test application.
        /// </summary>
        /// <param name="debugEnabled">True if the debugger should be started with and attached
        ///     to the app</param>
        /// <returns>A</returns>
        public IDisposable StartTestApp(bool debugEnabled)
            => debugEnabled ? (IDisposable) StartTestAppDebug() : StartTestApp();

        /// <summary>
        /// Starts the test app with no debugger attached.
        /// </summary>
        private TestAppWrapper StartTestApp() => TestAppWrapper.Create();

        /// <summary>
        /// Starts the test app with the debugger attached.
        /// </summary>
        private Agent StartTestAppDebug(AgentOptions options = null)
        {
            Agent agent = new Agent(options ?? Options);
            new Thread(() =>
            {
                agent.StartAndBlock();
            }).Start();
            return agent;
        }

        /// <summary>
        /// Private class to handle starting and shutting down of the test app.
        /// </summary>
        private class TestAppWrapper : IDisposable
        {
            public static TestAppWrapper Create()
            {
                var startInfo = ProcessUtils.GetStartInfoForInteractiveProcess(
                    "dotnet", $"{Utils.GetApplication()}", null);
                return new TestAppWrapper(Process.Start(startInfo));
            }

            private readonly Process _proc;

            private TestAppWrapper(Process proc)
            {
                _proc = proc;
            }

            /// <summary>
            /// Shuts down a running instance of the test app.
            /// This is done via a build in url that will kill the app when hit.
            /// When starting an app with the 'dotnet' command it will spawn a new process. This
            /// was more simple than trying to grab the child process id.
            /// </summary>
            public void Dispose()
            {
                _proc.Dispose();
                using (HttpClient client = new HttpClient())
                {
                    client.GetAsync(AppUrlShutdown).Wait();
                }
            }
        }
    }
}
