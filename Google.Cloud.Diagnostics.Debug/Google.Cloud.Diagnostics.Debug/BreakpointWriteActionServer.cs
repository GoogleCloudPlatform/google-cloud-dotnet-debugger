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
    public class BreakpointWriteActionServer : BreakpointActionServer
    {
        private readonly IDebuggerClient _client;
        private readonly BreakpointManager _breakpointManager;

        public BreakpointWriteActionServer(
            IBreakpointServer server, IDebuggerClient client) : base (server)
        {
            _client = GaxPreconditions.CheckNotNull(client, nameof(client));
            _breakpointManager = new BreakpointManager();
        }

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
                if (!string.IsNullOrWhiteSpace(breakpoint.Condition) || breakpoint.Expressions.Count() != 0)
                {
                    breakpoint.Status = Common.CreateStatusMessage(
                        Messages.CondExpNotSupported, isError: true);
                    breakpoint.IsFinalState = true;
                    _client.UpdateBreakpoint(breakpoint);
                }
                else
                {
                    _server.WriteBreakpointAsync(breakpoint.Convert()).Wait();
                }
            }
        }
    }
}
