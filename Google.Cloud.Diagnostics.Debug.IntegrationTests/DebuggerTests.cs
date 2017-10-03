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
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.IntegrationTests
{
    public class DebuggerTests
    {
        private const string _module = nameof(DebuggerTests);
        private readonly string _version;
        private readonly string _projectId;

        private readonly DebuggerPolling _polling;
        private readonly AgentOptions _options;

        public DebuggerTests()
        {
            _polling = new DebuggerPolling();
            _version = Guid.NewGuid().ToString();
            _projectId = Utils.GetProjectIdFromEnvironment();
            _options = new AgentOptions
            {
                Module = _module,
                Version = _version,
                ProjectId = _projectId,
                Debugger = Utils.GetDebugger(),
                Application = Utils.GetApplication(),
            };
        }

        [Fact]
        public async Task BreakpointHit()
        {
            new Thread(() =>
            {
                using (var agent = new Agent(_options))
                {
                    agent.StartAndBlock();
                }
            }).Start();

            var debuggee = _polling.GetDebuggee(_module, _version);
            var breakpoint = SetBreakpoint(debuggee.Id, "MainController.cs", 23);


            using (HttpClient client = new HttpClient())
            {
                await client.GetAsync("http://localhost:5000/");
            }


            var newBp = _polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

            // Check that the breakpoint has been hit.
            Assert.True(newBp.IsFinalState);
        }

        /// <summary>
        /// Set a breakpoint at a file and line for a given debuggee.
        /// </summary>
        private Debugger.V2.Breakpoint SetBreakpoint(string debuggeeId, string path, int line)
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
            return _polling.Client.GrpcClient.SetBreakpoint(request).Breakpoint;
        }
    }
}
