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
using System.IO;
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

        /// <summary>The underlying application, null if <see cref="_debugEnabled"/> is true.</summary>
        private readonly Process _app;

        /// <summary>The debugger agent, null if <see cref="_debugEnabled"/> is false.</summary>
        private readonly Process _agent;

        /// <summary>The path the  debugger to use for the test.</summary>
        private readonly string _debuggerPath;

        /// <summary>True if the debugger should be used. </summary>
        private readonly bool _debugEnabled;

        /// <summary>
        /// Create a new <see cref="TestApplication"/>.
        /// </summary>
        /// <param name="debugEnabled">True if a debugging should be enabled.</param>
        public TestApplication(bool debugEnabled)
        {
            _debugEnabled = debugEnabled;

            // Create a copy of the debugger executable with a unique name.
            // This is allows us to uniquely identify the running debugger to 
            // test the memory and CPU.  A little ugly but this is the simplest
            // way to make this work cross platform.
            var debugger = Utils.GetDebugger();
            var fileName = Utils.IsWindows ? $"{Version}.exe" : Version;
            var newDebugger = Path.Combine(Path.GetDirectoryName(debugger), fileName);           
            File.Copy(debugger, newDebugger);
            _debuggerPath = newDebugger;

            if (debugEnabled)
            {
                _agent = StartAppDebug();
            }
            else
            {
                _app = StartApp();
            }
        }

        /// <summary>
        /// Create a new <see cref="TestApplication"/> with no debugging.
        /// </summary>
        private Process StartApp()
        {
            var startInfo = ProcessUtils.GetStartInfoForInteractiveProcess(
                   "dotnet", $"{Utils.GetApplication()} --server.urls={AppUrlBase}", null);
            return Process.Start(startInfo);
        }

        /// <summary>
        /// Create a new <see cref="TestApplication"/> with no debugging.
        /// </summary>
        private Process StartAppDebug()
        {
            var startInfo = ProcessUtils.GetStartInfoForInteractiveProcess(
                  "dotnet", $"{Utils.GetAgent()} " +
                        $"--application-start-command=\"{GetStartCommand()}\" " +
                        $"--debugger={_debuggerPath} " +
                        $"--module={Module} " +
                        $"--version={Version} " +
                        $"--project-id={ProjectId}",
                  null);
            return Process.Start(startInfo);
        }

        /// <summary>
        /// Gets the command to start the test application.
        /// </summary>
        private string GetStartCommand() =>
            $"dotnet {Utils.GetApplication()} --server.urls={AppUrlBase}";

        /// <summary>
        /// Gets the process id for the running test application.
        /// </summary>
        public async Task<Process> GetApplicationProcess()
        {
            using (HttpClient client = new HttpClient())
            {
                HttpResponseMessage result = await client.GetAsync(AppUrlProcessId);
                var resultStr = await result.Content.ReadAsStringAsync();
                var processId = Int32.Parse(resultStr);
                return Process.GetProcessById(processId);
            }
        }

        /// <summary>
        /// Gets the debugger agent process or null if the debugger is not running.
        /// </summary>
        public Process GetAgentProcess() => _agent;

        /// <summary>
        /// Gets the debugger process or null if the debugger is not running.
        /// </summary>
        public Process GetDebuggerProcess()
        {
            if (!_debugEnabled)
            {
                return null;
            }

            var processes = Process.GetProcesses();
            foreach (var process in processes)
            {
                if (process.ProcessName.Contains(Version))
                {
                    return process;
                }
            }

            throw new InvalidOperationException("Debugger process not found.");
        }

        /// <summary>
        /// Shuts down a running instance of the test app.
        /// This is done via a build in url that will kill the app when hit.
        /// When starting an app with the 'dotnet' command it will spawn a new process. This
        /// was more simple than trying to grab the child process id.
        /// </summary>
        public void Dispose()
        {
            _app?.Dispose();
            _agent?.Dispose();
            using (HttpClient client = new HttpClient())
            {
                try
                {
                    client.GetAsync(AppUrlShutdown).Wait();
                }
                catch (AggregateException) { }
            }

            // TODO(talarico): This will fail as the debugger is not always shut down properly.
            //File.Delete(_debuggerPath);
        }
    }
}
