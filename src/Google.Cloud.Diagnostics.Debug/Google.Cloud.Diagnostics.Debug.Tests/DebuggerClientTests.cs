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
        private const string _debugId = "did";
        private const string _breakpointId = "bid";
        private const string _breakpointId2 = "bid2";
        private const string _projectId = "pid";
        private const string _module = "module";
        private const string _version = "version";
        private const string _waitToken = "wait-token-123";

        private readonly AgentOptions _options;
        private readonly Debuggee _debuggee;
        private readonly Debugger.V2.Breakpoint _breakpoint;
        private readonly Debugger.V2.Breakpoint _breakpoint2;
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
            _breakpoint2 = new Debugger.V2.Breakpoint
            {
                Id = _breakpointId2
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
            var bpReq = CreateBreakpointRequest();
            var bpResp = CreateBreakpointResponse(_waitToken, _breakpoint);
            var bpReq2 = CreateBreakpointRequest(_waitToken);
            var bpResp2 = CreateBreakpointResponse("some-token", _breakpoint2);

            _mockControllerClient.Setup(c => c.RegisterDebuggee(It.IsAny<Debuggee>(), null)).Returns(_response);
            _mockControllerClient.Setup(c => c.ListActiveBreakpoints(bpReq, null)).Returns(bpResp);
            _mockControllerClient.Setup(c => c.ListActiveBreakpoints(bpReq2, null)).Returns(bpResp2);

            _client.Register();
            var breakpoints = _client.ListBreakpoints();
            Assert.Equal(breakpoints, bpResp.Breakpoints);
            breakpoints = _client.ListBreakpoints();
            Assert.Equal(breakpoints, bpResp2.Breakpoints);
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

            var bpReq = CreateBreakpointRequest();
            var bpReq2 = CreateBreakpointRequest(debugId: response2.Debuggee.Id);
            var bpResp = CreateBreakpointResponse(DebuggerClient.InitialWaitToken, _breakpoint);

            _mockControllerClient.Setup(c => c.ListActiveBreakpoints(bpReq, null))
                .Throws(new RpcException(new Grpc.Core.Status(StatusCode.NotFound, "message")));
            _mockControllerClient.Setup(c => c.ListActiveBreakpoints(bpReq2, null))
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

        /// <summary>Create a <see cref="ListActiveBreakpointsRequest"/>.</summary>
        private ListActiveBreakpointsRequest CreateBreakpointRequest(
            string waitToken = DebuggerClient.InitialWaitToken, string debugId = _debugId)
        {
            return new ListActiveBreakpointsRequest
            {
                DebuggeeId = debugId,
                SuccessOnTimeout = true,
                WaitToken = waitToken,
            };
        }

        /// <summary>Create a <see cref="ListActiveBreakpointsResponse"/>.</summary>
        private ListActiveBreakpointsResponse CreateBreakpointResponse(string waitToken, Debugger.V2.Breakpoint breakpoint)
        {
            return new ListActiveBreakpointsResponse
            {
                Breakpoints =
                {
                   breakpoint
                },
                NextWaitToken = waitToken,
            };
        }
    }
}
