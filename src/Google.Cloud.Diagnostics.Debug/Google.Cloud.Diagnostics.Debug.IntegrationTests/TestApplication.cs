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

using System;
using System.Diagnostics;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;

namespace Google.Cloud.Diagnostics.Debug.IntegrationTests
{
    /// <summary>
    /// A class that represents the Test application.
    /// </summary>
    public class TestApplication : IDisposable
    {
        public static readonly string MainClass = "MainController.cs";
        public static readonly int HelloLine = 33;
        public static readonly int EchoTopLine = 38;
        public static readonly int EchoBottomLine = 49;
        public static readonly int PidLine = 59;

        /// <summary>The base url for the test application.</summary>
        public string AppUrlBase => $"http://localhost:{_port}";

        /// <summary>The url to forcibly shutdown the test application.</summary>
        public string AppUrlShutdown => $"{AppUrlBase}/Main/Shutdown";

        /// <summary>The url to echo the last piece of the path from the test application.</summary>
        public string AppUrlEcho => $"{AppUrlBase}/Main/Echo";

        /// <summary>The url to get the process id from the test application.</summary>
        public string AppUrlProcessId => $"{AppUrlBase}/Main/ProcessId";

        /// <summary>The module of the a debuggee.</summary>
        public readonly string Module = nameof(DebuggerTestBase);

        /// <summary>The version of the a debuggee.</summary>
        public readonly string Version = Guid.NewGuid().ToString();

        /// <summary>The Google CLoud Console project id to test with.</summary>
        public readonly string ProjectId = Utils.GetProjectIdFromEnvironment();

        /// <summary>The port to run the application on.</summary>
        private readonly int _port = Utils.GetNextPort();

        /// <summary>The underlying application.</summary>
        private readonly IDisposable _app;

        /// <summary>
        /// Create a new <see cref="TestApplication"/>.
        /// </summary>
        /// <param name="debugEnabled">True if a debugging should be enabled.</param>
        public TestApplication(bool debugEnabled)
        {
            _app = debugEnabled ? StartAppDebug() : StartApp();
        }

        /// <summary>
        /// Create a new <see cref="TestApplication"/> with no debugging.
        /// </summary>
        private IDisposable StartApp()
        {
            var startInfo = ProcessUtils.GetStartInfoForInteractiveProcess(
                   "dotnet", $"{Utils.GetApplication()} --server.urls={AppUrlBase}", null);
            return Process.Start(startInfo);
        }

        /// <summary>
        /// Create a new <see cref="TestApplication"/> with no debugging.
        /// </summary>
        private IDisposable StartAppDebug()
        {
            var options = new AgentOptions
            {
                Module = Module,
                Version = Version,
                ProjectId = ProjectId,
                Debugger = Utils.GetDebugger(),
                ApplicationStartCommand = GetStartCommand(),
            };
            Agent agent = new Agent(options);
            new Thread(() =>
            {
                agent.StartAndBlock();

            }).Start();
            return agent;
        }

        /// <summary>
        /// Gets the command to start the test application.
        /// </summary>
        private string GetStartCommand() =>
            $"dotnet {Utils.GetApplication()} --server.urls={AppUrlBase}";

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
        /// Shuts down a running instance of the test app.
        /// This is done via a build in url that will kill the app when hit.
        /// When starting an app with the 'dotnet' command it will spawn a new process. This
        /// was more simple than trying to grab the child process id.
        /// </summary>
        public void Dispose()
        {
            _app.Dispose();
            using (HttpClient client = new HttpClient())
            {
                try
                {
                    client.GetAsync(AppUrlShutdown).Wait();
                }
                catch (AggregateException) { }
            }
        }
    }
}
