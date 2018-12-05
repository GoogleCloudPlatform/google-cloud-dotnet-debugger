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

using Google.Api.Gax;
using Google.Cloud.Logging.V2;
using Google.Cloud.Logging.Type;
using System.Threading;
using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;
using Google.Api;
using System.Collections.Generic;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// An implementation of the <see cref="BreakpointActionServer"/> that
    /// handles reading breakpoints and reporting them to the debugger API.
    /// </summary>
    public class BreakpointReadActionServer : BreakpointActionServer
    {
        private readonly IDebuggerClient _debuggerClient;
        private readonly ILoggingClient _loggingClient;
        private readonly BreakpointManager _breakpointManager;

        /// <summary>
        /// Create a new <see cref="BreakpointReadActionServer"/>.
        /// </summary>
        /// <param name="server">The breakpoint server to communicate with.</param>
        /// <param name="cts"> A cancellation token source to cancel if the server receives a shutdown command.</param>
        /// <param name="client">The debugger client to send updated breakpoints to.</param>
        /// <param name="breakpointManager">A shared breakpoint manager.</param>
        public BreakpointReadActionServer(IBreakpointServer server, CancellationTokenSource cts,
            IDebuggerClient debuggerClient, ILoggingClient loggingClient,
            BreakpointManager breakpointManager) : base(server, cts)
        {
            _debuggerClient = GaxPreconditions.CheckNotNull(debuggerClient, nameof(debuggerClient));
            _loggingClient = GaxPreconditions.CheckNotNull(loggingClient, nameof(loggingClient));
            _breakpointManager = GaxPreconditions.CheckNotNull(breakpointManager, nameof(breakpointManager));
        }

        /// <summary>
        /// Blocks and reads a breakpoint from the <see cref="IBreakpointServer"/>
        /// and then sends the sends the breakpoint to the debugger API.
        /// </summary>
        internal override void MainAction()
        {
            Breakpoint readBreakpoint = _server.ReadBreakpointAsync().Result;
            if (readBreakpoint.KillServer)
            {
                _cts.Cancel();
                return;
            }
            StackdriverBreakpoint breakpoint = readBreakpoint.Convert();
            if (breakpoint.Action == StackdriverBreakpoint.Types.Action.Log)
            {
                _loggingClient.WriteLogEntry(breakpoint);
            }
            else
            {
                breakpoint.IsFinalState = true;
                _debuggerClient.UpdateBreakpoint(breakpoint);
            }
        }
    }
}
