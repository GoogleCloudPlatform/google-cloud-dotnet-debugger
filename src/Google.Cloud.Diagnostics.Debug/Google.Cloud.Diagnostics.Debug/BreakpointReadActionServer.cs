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
using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// An implementation of the <see cref="BreakpointActionServer"/> that
    /// handles reading breakpoints and reporting them to the debugger API.
    /// </summary>
    public class BreakpointReadActionServer : BreakpointActionServer
    {
        private readonly IDebuggerClient _client;
        private readonly BreakpointManager _breakpointManager;

        /// <summary>
        /// Create a new <see cref="BreakpointReadActionServer"/>.
        /// </summary>
        /// <param name="server">The breakpoint server to communicate with.</param>
        /// <param name="client">The debugger client to send updated breakpoints to.</param>
        /// <param name="breakpointManager">A shared breakpoint manager.</param>
        public BreakpointReadActionServer(IBreakpointServer server, IDebuggerClient client,
            BreakpointManager breakpointManager) : base(server)
        {
            _client = GaxPreconditions.CheckNotNull(client, nameof(client));
            _breakpointManager = GaxPreconditions.CheckNotNull(breakpointManager, nameof(breakpointManager));
        }

        /// <summary>
        /// Blocks and reads a breakpoint from the <see cref="IBreakpointServer"/>
        /// and then sends the sends the breakpoint to the debugger API.
        /// </summary>
        internal override void MainAction()
        {
            Breakpoint readBreakpoint = _server.ReadBreakpointAsync().Result;
            StackdriverBreakpoint breakpoint = readBreakpoint.Convert();
            breakpoint.IsFinalState = true;
            _client.UpdateBreakpoint(breakpoint);
        }
    }
}
