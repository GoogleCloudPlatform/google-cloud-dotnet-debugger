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
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;
using Grpc.Core;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// The <see cref="Agent"/> is the intermediary between a debugger process using the ICorDebug API and
    /// between the Stackdriver Debugger API.
    /// The agent will start the debugger process and have attach it to the users running process.  It will
    /// then register itself with the Stackdriver Debugger API and listen for new breakpoints to watch and
    /// report hit breakpoints.
    /// TODO(talarico): These docs need to be significantly expanded (use watchpoint).
    /// TODO(talarico): Add example of how to start this.
    /// TODO(talarico): Refactor this class it's become to random and some functions do too much.
    /// </summary>
    internal sealed class Agent : IDisposable
    {
        /// <summary>
        /// The minimum amount of time we will sleep when backing off failed RPC calls.
        /// The wait time may be higher if the user sets the option.
        /// </summary>
        private static readonly TimeSpan _minBackOffWaitTime = TimeSpan.FromSeconds(10);

        /// <summary>
        /// The maximum amount of time we will sleep when backing off failed RPC calls.
        /// The wait time may be higher if the user sets the option.
        /// </summary>
        private static readonly TimeSpan _maxBackOffWaitTime = TimeSpan.FromSeconds(10);

        private readonly AgentOptions _options;
        private readonly DebuggerClient _client;
        // Key is a location identifier (from path and line number) and value is ID of an existing breakpoint.
        private readonly ConcurrentDictionary<string, string> _breakpointLocationToId;
        private readonly CancellationTokenSource _cts;
        private readonly TaskCompletionSource<bool> _tcs;

        private Process _process;

        /// <summary>
        /// Create a new <see cref="Agent"/>.
        /// </summary>
        public Agent(AgentOptions options, Controller2Client controlClient = null)
        {
            _options = GaxPreconditions.CheckNotNull(options, nameof(options));
            _client = TryAction(() => new DebuggerClient(options, controlClient));
            _breakpointLocationToId = new ConcurrentDictionary<string, string>();
            _cts = new CancellationTokenSource();
            _tcs = new TaskCompletionSource<bool>();
        }

        /// <summary>
        /// Starts the <see cref="Agent"/>.
        /// </summary>
        public void StartAndBlock()
        {
            // Register the debuggee.
            TryAction(() => { _client.Register(); return true; });

            ProcessStartInfo startInfo = ProcessUtils.GetStartInfoForInteractiveProcess(
                _options.Debugger, _options.DebuggerArguments, null);
            _process = Process.Start(startInfo);

            // The write server needs to connect first due to initialization logic in the debugger.
            // TODO(talarico): Is this (^^) true? Should we change this logic?
            StartWriteLoopAsync(_cts.Token).Wait();
            StartReadLoopAsync(_cts.Token).Wait();

            // Start blocking.
            _tcs.Task.Wait();
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
        public Task StartWriteLoopAsync(CancellationToken cancellationToken)
        {
            TaskCompletionSource<bool> tcs = new TaskCompletionSource<bool>();
            new Thread(() =>
            {
                using (var server = new BreakpointServer(new NamedPipeServer()))
                {
                    server.WaitForConnectionAsync().Wait();
                    tcs.SetResult(true);
                    RepeatRpcAction(() =>
                    {
                        var serverBreakpoints = TryAction(() => _client.ListBreakpoints());

                        // Remove no longer active breakpoints from the debugger.
                        ISet<string> serverBreakpointIdentifiers =
                            new HashSet<string>(serverBreakpoints.Select(b => b.GetLocationIdentifier()));

                        // Retrieves all the identifiers from the active dictionary that the server no longer has.
                        IEnumerable<string> breakpointToBeRemovedIdentifiers = _breakpointLocationToId.Keys
                            .Where(identifier => !serverBreakpointIdentifiers.Contains(identifier));
                        foreach (string identifierToBeRemoved in breakpointToBeRemovedIdentifiers)
                        {
                            string[] locationStrings = identifierToBeRemoved.Split(':');
                            if (locationStrings.Length != 2)
                            {
                                throw new ArgumentException("Location must contains file name and line number.");
                            }

                            var location = new SourceLocation()
                            {
                                Path = locationStrings[0],
                                Line = Int32.Parse(locationStrings[1])
                            };

                            var breakpoint = new Breakpoint
                            {
                                Id = _breakpointLocationToId[identifierToBeRemoved],
                                Location = location,
                                Activated = false,
                            };
                            // TryRemove only fails if the key is no longer in the dictionary which
                            // is what we want anyway.
                            _breakpointLocationToId.TryRemove(identifierToBeRemoved, out _);
                            server.WriteBreakpointAsync(breakpoint).Wait();
                        }

                        // Send all breakpoints to the debugger.
                        // Even if the breakpoint is already set, the debugger will ignore it
                        // so we don't have to worry about sending over existing breakpoints.
                        foreach (StackdriverBreakpoint breakpoint in serverBreakpoints)
                        {
                            if (!string.IsNullOrWhiteSpace(breakpoint.Condition) || breakpoint.Expressions.Count() != 0)
                            {
                                breakpoint.Status = Common.CreateStatusMessage(
                                    Messages.CondExpNotSupported, isError: true);
                                breakpoint.IsFinalState = true;
                                TryAction(() => _client.UpdateBreakpoint(breakpoint));
                            }
                            else
                            {
                                _breakpointLocationToId[breakpoint.GetLocationIdentifier()] = breakpoint.Id;
                                server.WriteBreakpointAsync(breakpoint.Convert()).Wait();
                            }
                        }
                    }, TimeSpan.FromSeconds(_options.WaitTime), cancellationToken);
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
                using (var server = new BreakpointServer(new NamedPipeServer()))
                {
                    server.WaitForConnectionAsync().Wait();
                    tcs.SetResult(true);
                    RepeatRpcAction(() =>
                    {
                        Task<Breakpoint> breakpointFromDebugger = server.ReadBreakpointAsync();
                        breakpointFromDebugger.Wait();

                        Breakpoint readBreakpoint = breakpointFromDebugger.Result;
                        StackdriverBreakpoint breakpoint = readBreakpoint.Convert();
                        breakpoint.IsFinalState = true;

                        string locationIdentifier = readBreakpoint.GetLocationIdentifier();

                        // Retrieves the appropriate breakpoint id.
                        if (_breakpointLocationToId.ContainsKey(locationIdentifier))
                        {
                            breakpoint.Id = _breakpointLocationToId[locationIdentifier];
                        }
                        // Should not throw if the breakpoint is removed from list of active breakpoints.
                        // User may have removed it before the debugger has a chance to remove the breakpoint.

                        TryAction(() => _client.UpdateBreakpoint(breakpoint));
                    }, TimeSpan.Zero, cancellationToken);
                }
            }).Start();
            return tcs.Task;
        }

        /// <summary>
        /// Repeats an action that may throw and <see cref="RpcException"/>.
        /// If an <see cref="RpcException"/> is thrown the <paramref name="waitTime"/> will double
        /// until the action is successful (up to <see cref="_maxBackOffWaitTime"/>).  When the action
        /// is successful the wait between calls will return to the original amount.
        /// </summary>
        /// <param name="action">The action to repeated make.</param>
        /// <param name="waitTime">The time to wait between calls to the action.</param>
        /// <param name="cancellationToken">A token to signal this action should stop.</param>
        private void RepeatRpcAction(Action action, TimeSpan waitTime, CancellationToken cancellationToken)
        {
            TimeSpan originalWaitTime = waitTime;
            TimeSpan currentWaitTime = waitTime;
            while (!cancellationToken.IsCancellationRequested)
            {
                try
                {
                    action();
                    currentWaitTime = originalWaitTime;
                }
                catch (RpcException e)
                {
                    Console.WriteLine( $"RpcException with status code '{e.Status.StatusCode}' \n {e}");
                    if (currentWaitTime < _maxBackOffWaitTime)
                    {
                        currentWaitTime = TimeSpan.FromTicks(currentWaitTime.Ticks * 2);
                    }
                    currentWaitTime = currentWaitTime == TimeSpan.Zero ? _minBackOffWaitTime : currentWaitTime;
                }
                Thread.Sleep(currentWaitTime);
            }

        }

        /// <summary>
        /// Tries to perform an action. If a <see cref="DebuggeeDisabledException"/> is
        /// thrown complete the <see cref="_tcs"/> to signal the debugger should be shutdown.
        /// </summary>
        private T TryAction<T>(Func<T> func)
        {
            try
            {
                return func();
            }
            catch (DebuggeeDisabledException)
            {
                _tcs.SetResult(true);
                return default(T);
            }
        }

        // TODO(talarico): Move this out of this class.
        // TODO(talarico): Handle exceptions during startup.
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
