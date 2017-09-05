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
using Grpc.Core;
using Moq;
using System;
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class DebuggerClientTests
    {
        private static readonly string _debugId = "did";
        private static readonly string _breakpointId = "bid";
        private static readonly string _projectId = "pid";
        private static readonly string _module = "module";
        private static readonly string _version = "version";

        private readonly AgentOptions _options;
        private readonly Debuggee _debuggee;
        private readonly Debugger.V2.Breakpoint _breakpoint;
        private readonly RegisterDebuggeeResponse _response;
        private readonly Mock<Controller2Client> _mockControllerClient;
        private readonly DebuggerClient _client;
        
        public DebuggerClientTests()
        {
            _options = new AgentOptions
            {
                ProjectId = _projectId,
                Module = _module,
                Version = _version,
            };
            _debuggee = new Debuggee
            {
                Project = _projectId,
                Id = _debugId
            };
            _breakpoint = new Debugger.V2.Breakpoint
            {
                Id = _breakpointId
            };
            _response = new RegisterDebuggeeResponse
            {
                Debuggee = _debuggee
            };
            _mockControllerClient = new Mock<Controller2Client>();
            _client = new DebuggerClient(_options, _mockControllerClient.Object);
        }

        [Fact]
        public void Register()
        {
            _mockControllerClient.Setup(c => c.RegisterDebuggee(It.IsAny<Debuggee>(), null)).Returns(_response);
            _client.Register();
            _mockControllerClient.VerifyAll();
        }

        [Fact]
        public void Register_Disabled()
        {
            _debuggee.IsDisabled = true;
            _mockControllerClient.Setup(c => c.RegisterDebuggee(It.IsAny<Debuggee>(), null)).Returns(_response);
            Assert.Throws<DebuggeeDisabledException>(() => _client.Register());
            _mockControllerClient.VerifyAll();
        }

        [Fact]
        public void ListBreakpoints()
        {
            _mockControllerClient.Setup(c => c.RegisterDebuggee(It.IsAny<Debuggee>(), null)).Returns(_response);

            var bpResp = new ListActiveBreakpointsResponse
            {
                Breakpoints =
                {
                   _breakpoint
                }
            };
            _mockControllerClient.Setup(c => c.ListActiveBreakpoints(_debugId, null)).Returns(bpResp);

            _client.Register();
            var breakpoints = _client.ListBreakpoints();
            Assert.Equal(breakpoints, bpResp.Breakpoints);
            _mockControllerClient.VerifyAll();
        }

        [Fact]
        public void ListBreakpoints_NotRegistered() =>
            Assert.Throws<InvalidOperationException>(() => _client.ListBreakpoints());

        [Fact]
        public void ListBreakpoints_Retry()
        {
            var response2 = new RegisterDebuggeeResponse
            {
                Debuggee = new Debuggee
                {
                    Project = _projectId,
                    Id = _debugId + 2
                }
            };

            _mockControllerClient.SetupSequence(c => c.RegisterDebuggee(It.IsAny<Debuggee>(), null))
                .Returns(_response)
                .Returns(response2);

            var bpResp = new ListActiveBreakpointsResponse
            {
                Breakpoints =
                {
                    _breakpoint
                }
            };
            _mockControllerClient.Setup(c => c.ListActiveBreakpoints(_debugId, null))
                .Throws(new RpcException(new Grpc.Core.Status(StatusCode.NotFound, "message")));
            _mockControllerClient.Setup(c => c.ListActiveBreakpoints(response2.Debuggee.Id, null))
                .Returns(bpResp);

            _client.Register();
            var breakpoints = _client.ListBreakpoints();
            Assert.Equal(breakpoints, bpResp.Breakpoints);
            _mockControllerClient.VerifyAll();
        }

        [Fact]
        public void UpdateBreakpoint()
        {
            _mockControllerClient.Setup(c => c.RegisterDebuggee(It.IsAny<Debuggee>(), null)).Returns(_response);
            _mockControllerClient.Setup(c => c.UpdateActiveBreakpoint(_debugId, _breakpoint, null))
                .Returns(new UpdateActiveBreakpointResponse());

            _client.Register();
            var breakpoints = _client.UpdateBreakpoint(_breakpoint);
            _mockControllerClient.VerifyAll();
        }

        [Fact]
        public void UpdateBreakpoint_NotRegistered() =>
            Assert.Throws<InvalidOperationException>(() => _client.UpdateBreakpoint(new Debugger.V2.Breakpoint()));

        [Fact]
        public void UpdateBreakpoint_Retry()
        {
            var response2 = new RegisterDebuggeeResponse
            {
                Debuggee = new Debuggee
                {
                    Project = _projectId,
                    Id = _debugId + 2
                }
            };

            _mockControllerClient.SetupSequence(c => c.RegisterDebuggee(It.IsAny<Debuggee>(), null))
                .Returns(_response)
                .Returns(response2);

            _mockControllerClient.Setup(c => c.UpdateActiveBreakpoint(_debugId, _breakpoint, null))
                .Throws(new RpcException(new Grpc.Core.Status(StatusCode.NotFound, "message")));
            _mockControllerClient.Setup(c => c.UpdateActiveBreakpoint(response2.Debuggee.Id, _breakpoint, null))
                .Returns(new UpdateActiveBreakpointResponse());

            _client.Register();
            var breakpoints = _client.UpdateBreakpoint(_breakpoint);
            _mockControllerClient.VerifyAll();
        }
    }
}
