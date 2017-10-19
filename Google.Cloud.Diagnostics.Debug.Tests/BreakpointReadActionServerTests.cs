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
using System.Threading;
using System.Threading.Tasks;
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class BreakpointReadActionServerTests
    {
        private readonly Mock<IBreakpointServer> _mockBreakpointServer;
        private readonly Mock<IDebuggerClient> _mockDebuggerClient;
        private readonly BreakpointReadActionServer _server;

        public BreakpointReadActionServerTests()
        {
            _mockBreakpointServer = new Mock<IBreakpointServer>();
            _mockDebuggerClient = new Mock<IDebuggerClient>();
            _server = new BreakpointReadActionServer(
                _mockBreakpointServer.Object, _mockDebuggerClient.Object);
        }

        [Fact]
        public void MainAction()
        {
            var breakpoint = new Breakpoint
            {
                Id = "some-id",
            };
            _mockBreakpointServer.Setup(s => s.ReadBreakpointAsync(It.IsAny<CancellationToken>()))
                .Returns(Task.FromResult(breakpoint));
            _server.MainAction();

            var sdBreakpoint = breakpoint.Convert();
            sdBreakpoint.IsFinalState = true;
            _mockDebuggerClient.Verify(c => c.UpdateBreakpoint(sdBreakpoint), Times.Once);
        }
    }
}
