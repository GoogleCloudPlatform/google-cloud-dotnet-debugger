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

using Moq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Xunit;

using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class BreakpointWriteActionServerTests
    {
        private readonly Mock<IBreakpointServer> _mockBreakpointServer;
        private readonly Mock<IDebuggerClient> _mockDebuggerClient;
        private readonly CancellationTokenSource _cts;
        private readonly BreakpointWriteActionServer _server;
        private readonly BreakpointManager _breakpointManager;

        public BreakpointWriteActionServerTests()
        {
            _mockBreakpointServer = new Mock<IBreakpointServer>();
            _mockDebuggerClient = new Mock<IDebuggerClient>();
            _breakpointManager = new BreakpointManager();
            _cts = new CancellationTokenSource();
            _server = new BreakpointWriteActionServer(_mockBreakpointServer.Object,
                _cts, _mockDebuggerClient.Object, _breakpointManager);

            _mockBreakpointServer.Setup(s => s.WriteBreakpointAsync(
                It.IsAny<Breakpoint>(), It.IsAny<CancellationToken>()))
                    .Returns(Task.FromResult(true));
        }

        [Fact]
        public void MainAction_NoBreakpoints()
        {
            var breakpoints = CreateBreakpoints(0);
            _mockDebuggerClient.Setup(c => c.ListBreakpoints()).Returns(breakpoints);
            _server.MainAction();

            _mockDebuggerClient.Verify(c => c.ListBreakpoints(), Times.Once);
            _mockDebuggerClient.Verify(c => c.UpdateBreakpoint(It.IsAny<StackdriverBreakpoint>()), Times.Never);
            _mockBreakpointServer.Verify(s => s.WriteBreakpointAsync(
                It.IsAny<Breakpoint>(), It.IsAny<CancellationToken>()), Times.Never);
        }

        [Fact]
        public void MainAction_WaitExpired()
        {
            var breakpoints = CreateBreakpoints(0);
            _mockDebuggerClient.Setup(c => c.ListBreakpoints()).Returns((IEnumerable<Debugger.V2.Breakpoint>) null);
            _server.MainAction();

            _mockDebuggerClient.Verify(c => c.ListBreakpoints(), Times.Once);
            _mockDebuggerClient.Verify(c => c.UpdateBreakpoint(It.IsAny<StackdriverBreakpoint>()), Times.Never);
            _mockBreakpointServer.Verify(s => s.WriteBreakpointAsync(
                It.IsAny<Breakpoint>(), It.IsAny<CancellationToken>()), Times.Never);
        }

        [Fact]
        public void MainAction_NewBreakpoint()
        {
            var breakpoints = CreateBreakpoints(1);
            _mockDebuggerClient.Setup(c => c.ListBreakpoints()).Returns(breakpoints);
            _server.MainAction();

            _mockDebuggerClient.Verify(c => c.ListBreakpoints(), Times.Once);
            _mockDebuggerClient.Verify(c => c.UpdateBreakpoint(It.IsAny<StackdriverBreakpoint>()), Times.Never);
            _mockBreakpointServer.Verify(s => s.WriteBreakpointAsync(
                breakpoints.Single().Convert(), It.IsAny<CancellationToken>()), Times.Once);
        }

        [Fact]
        public void MainAction_LogPoint()
        {
            var breakpoints = CreateBreakpoints(1);
            breakpoints.Single().Action = StackdriverBreakpoint.Types.Action.Log;
            _mockDebuggerClient.Setup(c => c.ListBreakpoints()).Returns(breakpoints);
            _server.MainAction();

            _mockDebuggerClient.Verify(c => c.ListBreakpoints(), Times.Once);
            _mockDebuggerClient.Verify(c => c.UpdateBreakpoint(
                Match.Create(GetErrorMatcher("0", Messages.LogPointNotSupported))), Times.Once);
        }


        [Fact]
        public void MainAction_Expression()
        {
            var breakpoints = CreateBreakpoints(1);
            breakpoints.Single().Expressions.Add("x");
            _mockDebuggerClient.Setup(c => c.ListBreakpoints()).Returns(breakpoints);
            _server.MainAction();

            _mockDebuggerClient.Verify(c => c.ListBreakpoints(), Times.Once);
            _mockDebuggerClient.Verify(c => c.UpdateBreakpoint(
                Match.Create(GetErrorMatcher("0", Messages.ExpressionsNotSupported))), Times.Once);
        }

        [Fact]
        public void MainAction_RemoveBreakpoint()
        {
            var breakpoints = CreateBreakpoints(1);
            _mockDebuggerClient.Setup(c => c.ListBreakpoints()).Returns(breakpoints);
            _server.MainAction();

            _mockDebuggerClient.Setup(c => c.ListBreakpoints()).Returns(CreateBreakpoints(0));
            _server.MainAction();

            _mockDebuggerClient.Verify(c => c.ListBreakpoints(), Times.Exactly(2));
            _mockDebuggerClient.Verify(c => c.UpdateBreakpoint(It.IsAny<StackdriverBreakpoint>()), Times.Never);
            breakpoints.Single().IsFinalState = true;
            _mockBreakpointServer.Verify(s => s.WriteBreakpointAsync(
                breakpoints.Single().Convert(), It.IsAny<CancellationToken>()), Times.Once);
        }

        [Fact]
        public void MainAction_MultipleBreakpoints()
        {
            var breakpoints = CreateBreakpoints(5);
            breakpoints[2].Condition = "x == y";
            _mockDebuggerClient.Setup(c => c.ListBreakpoints()).Returns(breakpoints);
            _server.MainAction();

            _mockBreakpointServer.Verify(s => s.WriteBreakpointAsync(
                It.IsAny<Breakpoint>(), It.IsAny<CancellationToken>()), Times.Exactly(5));

            _mockDebuggerClient.Reset();
            _mockBreakpointServer.Reset();
            _mockDebuggerClient.Setup(c => c.ListBreakpoints()).Returns(breakpoints.GetRange(4, 1));
            _server.MainAction();

            _mockDebuggerClient.Verify(c => c.UpdateBreakpoint(It.IsAny<StackdriverBreakpoint>()), Times.Never);
            _mockBreakpointServer.Verify(s => s.WriteBreakpointAsync(
                Match.Create((Breakpoint b) => !b.Activated), It.IsAny<CancellationToken>()), Times.Exactly(4));
        }

        /// <summary>
        /// Creates a matcher that will match a <see cref="StackdriverBreakpoint"/>
        /// that has an error status.
        /// </summary>
        private Predicate<StackdriverBreakpoint> GetErrorMatcher(string id, string errorMessage)
        {
            return (b) =>
                b.IsFinalState && b.Id == id && b.Status.IsError &&
                b.Status.Description.Format == errorMessage;
        }

        /// <summary>
        /// Create a list of <see cref="StackdriverBreakpoint"/>s.
        /// </summary>
        private List<StackdriverBreakpoint> CreateBreakpoints(int numBreakpoints)
        {
            List<StackdriverBreakpoint> breakpoints = new List<StackdriverBreakpoint>();
            for (int i = 0; i < numBreakpoints; i++)
            {
                breakpoints.Add(new StackdriverBreakpoint
                {
                    Id = $"{i}",
                    Location = new Debugger.V2.SourceLocation
                    {
                        Path = $"{i}/{i}",
                        Line = i
                    }
                });
            }
            return breakpoints;
        }
    }
}
