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
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// The <see cref="Debuglet"/> is the intermediary between a debugger process using the ICorDebug API and
    /// between the Stackdriver Debugger API.
    /// The debuglet will start the debugger process and have attach it to the users running process.  It will
    /// then register itself with the Stackdriver Debugger API and listen for new breakpoints to watch and
    /// report hit breakpoints.
    /// TODO(talarico): These docs need to be significantly expanded (use watchpoint).
    /// TODO(talarico): Add example of how to start this.
    /// </summary>
    public sealed class Debuglet : IDisposable
    {
        private readonly DebugletOptions _options;
        private readonly Controller2Client _controlClient;
        private readonly IDictionary<string, StackdriverBreakpoint> _breakpoints;
        private readonly CancellationTokenSource _cts;

        private Debuggee _debuggee;
        private Process _process;

        private Debuglet(DebugletOptions options, Debugger2Client debugClient = null, Controller2Client controlClient = null)
        {
            _options = GaxPreconditions.CheckNotNull(options, nameof(options));
            _controlClient = controlClient ?? Controller2Client.Create();
            _breakpoints = new Dictionary<string, StackdriverBreakpoint>();
            _cts = new CancellationTokenSource();
        }

        /// <summary>
        /// Starts the <see cref="Debuglet"/>.
        /// </summary>
        private void Start()
        {
            var debuggee = DebuggeeUtils.CreateDebuggee(_options.ProjectId, _options.Module, _options.Version);
            // TODO(talarico): Check return value here.
            // TODO(talarico): Poll to see if we need to shut this off.
            _debuggee = _controlClient.RegisterDebuggee(debuggee).Debuggee;

            ProcessStartInfo startInfo = ProcessUtils.GetStartInfoForInteractiveProcess(
                _options.Debugger, _options.ProcessId, null);
            _process = Process.Start(startInfo);

            // The write server needs to connect first due to initialization logic in the debugger.
            // TODO(talarico): Is this (^^) true? Should we change this logic?
            StartWriteLoopAsync(_cts.Token).Wait();
            StartReadLoopAsync(_cts.Token).Wait();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            // TODO(talarico): Be sure to signal the debugger to detach.
            _process?.Dispose();
            _cts.Cancel();
        }

        /// <summary>
        /// Starts a new <see cref="Thread"/>>, will poll the Stackdriver Debugger API for new
        /// breakpoints and reports them to the debugger via a <see cref="NamedPipeServer"/>.
        /// </summary>
        /// <returns>A task representing the asynchronous operation which will be completed when the
        /// named pipe server is connected.</returns>
        private Task StartWriteLoopAsync(CancellationToken cancellationToken)
        {
            TaskCompletionSource<bool> tcs = new TaskCompletionSource<bool>();
            new Thread(() =>
            {
                using (var server = new BreakpointServer(new NamedPipeServer()))
                {
                    server.WaitForConnectionAsync().Wait();
                    tcs.SetResult(true);
                    while (!cancellationToken.IsCancellationRequested)
                    {
                        var breakpoints = _controlClient.ListActiveBreakpoints(_debuggee.Id).Breakpoints;

                        // TODO(talarico): Do we need to wipe out old breakpoints that were hit?

                        var newBreakpoints = breakpoints.Where(b => !_breakpoints.ContainsKey(b.Id));
                        foreach (StackdriverBreakpoint breakpoint in newBreakpoints)
                        {
                            _breakpoints.Add(breakpoint.Id, breakpoint);
                            server.WriteBreakpointAsync(breakpoint.Convert()).Wait();
                        }
                    }
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
        private Task StartReadLoopAsync(CancellationToken cancellationToken)
        {
            TaskCompletionSource<bool> tcs = new TaskCompletionSource<bool>();
            new Thread(() =>
            {
                using (var server = new BreakpointServer(new NamedPipeServer()))
                {
                    server.WaitForConnectionAsync().Wait();
                    tcs.SetResult(true);
                    while (!cancellationToken.IsCancellationRequested)
                    {
                        Task<Breakpoint> breakpointFromCpp = server.ReadBreakpointAsync();
                        breakpointFromCpp.Wait();

                        var readBreakpoint = breakpointFromCpp.Result;

                        var breakpoint = readBreakpoint.Convert();
                        breakpoint.IsFinalState = true;
                        _controlClient.UpdateActiveBreakpoint(_debuggee.Id, breakpoint);
                    }

                }
            }).Start();
            return tcs.Task;
        }



        // TODO(talarico): Move this out of this class.
        // TODO(talarico): Handle exceptions during startup.
        /// <summary>
        /// Starts a debuglet and blocks the terminal.
        /// </summary>
        /// <example>
        /// PS> $app = Start-Process dotnet .\bin\Debug\netcoreapp1.1\ConsoleApp.dll -PassThru
        /// PS> dotnet .\Google.Cloud.Diagnostics.Debug.dll --debugger .\GoogleCloudDebugger.exe 
        ///         --process-id $app.Id --project-id your-pid --module some-app --version current-version
        /// </example>
        public static void Main(string[] args)
        {
            var options = DebugletOptions.Parse(args);
            using (var debuglet = new Debuglet(options))
            {
                debuglet.Start();
                // TODO(talarico): What's the best way to do this?
                TaskCompletionSource<bool> tcs = new TaskCompletionSource<bool>();
                tcs.Task.Wait();
            }
        }
    }
}
