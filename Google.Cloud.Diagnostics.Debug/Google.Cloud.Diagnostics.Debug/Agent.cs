// Copyright 2015-2016 Google Inc. All Rights Reserved.
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

using Google.Api.Gax;
using Google.Cloud.Debugger.V2;
using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using System.Management;


namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// The <see cref="Agent"/> is the intermediary between a debugger process using the ICorDebug API and
    /// between the Stackdriver Debugger API.
    /// The agent will start the debugger process and have attach it to the users running process.  It will
    /// then register itself with the Stackdriver Debugger API and listen for new breakpoints to watch and
    /// report hit breakpoints.
    /// TODO(talarico): These docs need to be significantly expanded (use watchpoint).
    /// </summary>
    internal sealed class Agent : IDisposable
    {
        private readonly AgentOptions _options;
        private readonly DebuggerClient _client;
        private readonly CancellationTokenSource _cts;
        private readonly TaskCompletionSource<bool> _tcs;
        private readonly BreakpointManager _breakpointManager;

        private Process _process;

        /// <summary>
        /// Create a new <see cref="Agent"/>.
        /// </summary>
        public Agent(AgentOptions options, Controller2Client controlClient = null)
        {
            _options = GaxPreconditions.CheckNotNull(options, nameof(options));
            _client = new DebuggerClient(options, controlClient);
            _cts = new CancellationTokenSource();
            _tcs = new TaskCompletionSource<bool>();
            _breakpointManager = new BreakpointManager();
        }

        /// <summary>
        /// Starts the <see cref="Agent"/>.
        /// </summary>
        public void StartAndBlock()
        {
            // Register the debuggee.
            TryAction(() => _client.Register());

            // Start the debugger.
            ProcessStartInfo startInfo = ProcessUtils.GetStartInfoForInteractiveProcess(
                _options.Debugger, _options.DebuggerArguments, null);
            _process = Process.Start(startInfo);

            // The write server needs to connect first due to initialization logic in the debugger.
            StartWriteLoopAsync(_cts.Token).Wait();
            StartReadLoopAsync(_cts.Token).Wait();

            // Start blocking.
            _tcs.Task.Wait();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            var processes = Process.GetProcesses();
            foreach (var process in processes)
            {
            //    process.Get
            }

            _process?.Kill();
            _process?.Dispose();  
            _cts.Cancel();
            Console.WriteLine("*********************END Dispose");
        }

        /// <summary>
        /// Starts a new <see cref="Thread"/>, will poll the Stackdriver Debugger API for new
        /// breakpoints and reports them to the debugger via a <see cref="NamedPipeServer"/>.
        /// </summary>
        /// <returns>A task representing the asynchronous operation which will be completed when the
        /// named pipe server is connected.</returns>
        public Task StartWriteLoopAsync(CancellationToken cancellationToken)
        {
            TaskCompletionSource<bool> tcs = new TaskCompletionSource<bool>();
            new Thread(() =>
            {
                var breakpointServer = new BreakpointServer(new NamedPipeServer());
                using (var server = new BreakpointWriteActionServer(breakpointServer, _client, _breakpointManager))
                {
                    TryAction(() =>
                    {
                        server.WaitForConnection();
                        tcs.SetResult(true);
                        server.StartActionLoop(TimeSpan.FromSeconds(_options.WaitTime), cancellationToken);
                    });
                    Breakpoint breakpoint = new Breakpoint
                    {
                        KillServer = true
                    };
                    breakpointServer.WriteBreakpointAsync(breakpoint).Wait();
                }
            }).Start();
            return tcs.Task;
        }

        /// <summary>
        /// Starts a new <see cref="Thread"/>>, will poll the debugger (via a <see cref="NamedPipeServer"/>)
        /// for hit breakpoints and reports them to the Stackdriver Debugger API.
        /// </summary>
        /// <returns>A task representing the asynchronous operation which will be completed when the
        /// named pipe server is connected.</returns>
        public Task StartReadLoopAsync(CancellationToken cancellationToken)
        {
            TaskCompletionSource<bool> tcs = new TaskCompletionSource<bool>();
            new Thread(() =>
            {
                var breakpointServer = new BreakpointServer(new NamedPipeServer());
                using (var server = new BreakpointReadActionServer(breakpointServer, _client, _breakpointManager))
                {
                    TryAction(() => 
                    { 
                        server.WaitForConnection();
                        tcs.SetResult(true);
                        server.StartActionLoop(TimeSpan.Zero, cancellationToken);
                    });
                }
            }).Start();
            return tcs.Task;
        }

        /// <summary>
        /// Tries to perform an action. If a <see cref="DebuggeeDisabledException"/> is
        /// thrown complete the <see cref="_tcs"/> to signal the debugger should be shutdown.
        /// </summary>
        private void TryAction(Action func)
        {
            try
            {
                 func();
            }
            catch (DebuggeeDisabledException)
            {
                _tcs.SetResult(true);
            }
        }

        // TODO(talarico): Move this out of this class.
        /// <summary>
        /// Starts a agent and blocks the terminal.
        /// </summary>
        /// <example>
        /// PS> dotnet .\Google.Cloud.Diagnostics.Debug.dll --debugger .\GoogleCloudDebugger.exe 
        ///         --application .\bin\Debug\netcoreapp1.1\ConsoleApp.dll
        ///         --project-id your-pid --module some-app --version current-version
        /// </example>
        public static void Main(string[] args)
        {
            var options = AgentOptions.Parse(args);
            using (var agent = new Agent(options))
            {
                agent.StartAndBlock();
            }
        }
    }
}
