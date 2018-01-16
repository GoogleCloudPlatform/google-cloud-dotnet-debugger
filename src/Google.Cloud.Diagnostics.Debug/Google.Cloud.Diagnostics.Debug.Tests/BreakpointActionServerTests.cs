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

using Grpc.Core;
using Moq;
using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class BreakpointActionServerTests
    {
        // The tolerance for number of calls to the main action.
        private static readonly int _tolerance = 10;
        private static readonly TimeSpan _minBackOffWaitTime = TimeSpan.FromMilliseconds(10);
        private static readonly TimeSpan _maxBackOffWaitTime = TimeSpan.FromMilliseconds(100);

        private readonly Mock<IBreakpointServer> _mockBreakpointServer;
        private readonly FakeBreakpointActionServer _fakeActionServer;

        public BreakpointActionServerTests()
        {
            _mockBreakpointServer = new Mock<IBreakpointServer>();
            _mockBreakpointServer.Setup(s => s.WaitForConnectionAsync()).Returns(Task.FromResult(true));
            _fakeActionServer = new FakeBreakpointActionServer(_mockBreakpointServer.Object);
        }

        [Fact]
        public void Dispose()
        {
            _fakeActionServer.Dispose();
            _mockBreakpointServer.Verify(s => s.Dispose(), Times.Once);
        }

        [Fact]
        public void WaitForConnection()
        {
            _fakeActionServer.WaitForConnection();
            _mockBreakpointServer.Verify(s => s.WaitForConnectionAsync(), Times.Once);
        }

        [Fact]
        public void StartActionLoop()
        {
            var waitTime = TimeSpan.FromMilliseconds(10);
            var elapsedMilliseconds = RunLoop(TimeSpan.FromSeconds(1), waitTime);
            var estimatedCalls = elapsedMilliseconds / waitTime.TotalMilliseconds;
            Assert.InRange(_fakeActionServer.Counter, estimatedCalls - _tolerance, estimatedCalls + _tolerance);
        }

        [Fact]
        public void StartActionLoop_Throw()
        {
            var waitTime = TimeSpan.FromMilliseconds(10);
            _fakeActionServer.ThrowTimes = 100000;
            var elapsedMilliseconds = RunLoop(TimeSpan.FromSeconds(1), waitTime);

            // We only have 3 calls that will not be at the max wait out. Total non standard
            // call time is 140.  (300 - 140)/100 = 1.6 =~ 2
            // Call wait times (20, 40, 80, 100, 100, 100...)
            var estimatedCalls = (elapsedMilliseconds / _maxBackOffWaitTime.TotalMilliseconds) - 2;
            Assert.InRange(_fakeActionServer.Counter, estimatedCalls - _tolerance, estimatedCalls + _tolerance);
        }

        [Fact]
        public void StartActionLoop_ThrowAndRecover()
        {
            var waitTime = TimeSpan.FromMilliseconds(10);
            _fakeActionServer.ThrowTimes = 4;
            var elapsedMilliseconds = RunLoop(TimeSpan.FromSeconds(1), waitTime);

            // We only have 4 calls that will not be at the min wait out.  Total non standard
            // call time is 240.  (240 - (4 * 10)) / 10 = 20;
            // Call wait times (20, 40, 80, 100, 10, 10...)
            var estimatedCalls = (elapsedMilliseconds / waitTime.TotalMilliseconds) - 20;
            Assert.InRange(_fakeActionServer.Counter, estimatedCalls - _tolerance, estimatedCalls + _tolerance);
        }

        /// <summary>
        /// Run the main loop for a given amount of time.
        /// </summary>
        /// <param name="testTime">The amount of time to run the main action loop for.</param>
        /// <param name="waitTime">The wait time to be passed to the server.</param>
        /// <returns>The actual time elapsed in milliseconds during the run of the test.</returns>
        private long RunLoop(TimeSpan testTime, TimeSpan waitTime)
        {
            CancellationTokenSource cts = new CancellationTokenSource();
            Stopwatch timer = new Stopwatch();
            new Thread(() =>
            {
                timer.Start();
                _fakeActionServer.StartActionLoop(waitTime, cts.Token);
                timer.Stop();
            }).Start();
            Thread.Sleep(testTime);
            cts.Cancel();
            return timer.ElapsedMilliseconds;
        }

        /// <summary>
        /// A fake <see cref="BreakpointActionServer"/>.
        /// </summary>
        private class FakeBreakpointActionServer : BreakpointActionServer
        {
            // The number of times the MainAction has been called.
            public int Counter = 0;

            // The number of times to throw exceptions during a call to the
            // MainAction function.
            public int ThrowTimes = 0;

            public FakeBreakpointActionServer(IBreakpointServer server) : 
                base(server, _minBackOffWaitTime, _maxBackOffWaitTime) { }

            internal override void MainAction()
            {
                Counter++;
                if (ThrowTimes > 0)
                {
                    ThrowTimes--;
                    throw new RpcException(Grpc.Core.Status.DefaultCancelled);
                }
            }
        }
    }
}
