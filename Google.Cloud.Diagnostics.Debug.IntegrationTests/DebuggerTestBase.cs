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

        /// <summary>The number of requests to test against. </summary>
        public static readonly int NumberOfRequest = 100;

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
                ApplicationPath = Utils.GetApplication(),
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
        /// <param name="waitForStart">Optional. True if this method should block until the
        ///     application is started and can be queried.  Defaults to true.</param>
        /// <returns>A test application.</returns>
        public IDisposable StartTestApp(bool debugEnabled, bool waitForStart = true)
        { 
            var app = debugEnabled ? TestAppWrapper.CreateDebug(Options) : TestAppWrapper.Create();

            if (waitForStart)
            {
                using (HttpClient client = new HttpClient())
                {
                    // Allow the app a chance to start up as it may not start
                    // right away.
                    for (int i = 0; i < 5; i++)
                    {
                        try
                        {
                            client.GetAsync(AppUrlBase).Wait();
                            break;
                        }
                        catch (AggregateException) when (i < 4)
                        {
                            Thread.Sleep(TimeSpan.FromSeconds(5));
                        }
                    }
                }
            }
            return app;
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
            
            public static TestAppWrapper CreateDebug(AgentOptions options)
            {
                Agent agent = new Agent(options);
                new Thread(() =>
                {
                    agent.StartAndBlock();

                }).Start();
                return new TestAppWrapper(agent);
            }

            private readonly IDisposable _disposable;

            private TestAppWrapper(IDisposable disposable)
            {
                _disposable = disposable;
            }

            /// <summary>
            /// Shuts down a running instance of the test app.
            /// This is done via a build in url that will kill the app when hit.
            /// When starting an app with the 'dotnet' command it will spawn a new process. This
            /// was more simple than trying to grab the child process id.
            /// </summary>
            public void Dispose()
            {
                _disposable.Dispose();
                using (HttpClient client = new HttpClient())
                {
                    client.GetAsync(AppUrlShutdown).Wait();
                }
            }
        }
    }
}
