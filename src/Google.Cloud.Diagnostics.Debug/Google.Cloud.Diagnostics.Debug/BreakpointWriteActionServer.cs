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
using System.Linq;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// An implementation of the <see cref="BreakpointActionServer"/> that
    /// handles reading breakpoints from the debugger API and reporting
    /// them to the <see cref="IBreakpointServer"/>.
    /// </summary>
    public class BreakpointWriteActionServer : BreakpointActionServer
    {
        private readonly IDebuggerClient _client;
        private readonly BreakpointManager _breakpointManager;

        /// <summary>
        /// Create a new <see cref="BreakpointWriteActionServer"/>.
        /// </summary>
        /// <param name="server">The breakpoint server to communicate with.</param>
        /// <param name="client">The debugger client to send updated breakpoints to.</param>
        /// <param name="breakpointManager">A shared breakpoint manager.</param>
        public BreakpointWriteActionServer(IBreakpointServer server, IDebuggerClient client, 
            BreakpointManager breakpointManager) : base (server)
        {
            _client = GaxPreconditions.CheckNotNull(client, nameof(client));
            _breakpointManager = GaxPreconditions.CheckNotNull(breakpointManager, nameof(breakpointManager));
        }

        /// <summary>
        /// Lists breakpoints from the debugger API.  Stale breakpoints are removed,
        /// new breakpoints are sent to the <see cref="IBreakpointServer"/> and 
        /// breakpoints that cannot be processed are returned with an error.
        /// </summary>
        internal override void MainAction()
        {
            var serverBreakpoints = _client.ListBreakpoints();
            var bpmResponse = _breakpointManager.UpdateBreakpoints(serverBreakpoints);

            foreach (var breakpointToBeRemoved in bpmResponse.Removed)
            {
                var breakpoint = breakpointToBeRemoved.Convert();
                breakpoint.Activated = false;
                _server.WriteBreakpointAsync(breakpoint).Wait();
            }

            foreach (var breakpoint in bpmResponse.New)
            {
                _server.WriteBreakpointAsync(breakpoint.Convert()).Wait();
            }
        }
    }
}
