// Copyright 2015-2016 Google Inc. All Rights Reserved.
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
using Grpc.Core;
using System;
using System.Threading;
using System.Threading.Tasks;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// A wrapper around an <see cref="IBreakpointServer"/> to help repeat an
    /// action against that server with a potential delay and retry logic for
    /// <see cref="RpcException"/>s.
    /// </summary>
    public abstract class BreakpointActionServer : IDisposable
    {
        /// <summary>
        /// The default minimum amount of time we will sleep when backing off failed RPC calls.
        /// </summary>
        private static readonly TimeSpan _defaultMinBackOffWaitTime = TimeSpan.FromSeconds(1);

        /// <summary>
        /// The default maximum amount of time we will sleep when backing off failed RPC calls.
        /// </summary>
        private static readonly TimeSpan _defaultMaxBackOffWaitTime = TimeSpan.FromSeconds(10);

        /// <summary>
        /// The minimum amount of time we will sleep when backing off failed RPC calls.
        /// </summary>
        private readonly TimeSpan _minBackOffWaitTime;

        /// <summary>
        /// The maximum amount of time we will sleep when backing off failed RPC calls.
        /// </summary>
        private readonly TimeSpan _maxBackOffWaitTime;

        /// <summary>
        /// The breakpoint server.
        /// </summary>
        protected readonly IBreakpointServer _server;

        /// <summary>
        /// A cancellation token source to cancel if the server receives a shutdown command.
        /// </summary>
        protected readonly CancellationTokenSource _cts;

        /// <summary>
        /// Create a new <see cref="BreakpointActionServer"/>.
        /// </summary>
        /// <param name="server">The breakpoint server to communicate with.</param>
        /// <param name="cts"> A cancellation token source to cancel if the server receives a shutdown command.</param>
        /// <param name="minBackOffWaitTime">The minimum amount of time we will sleep when backing off failed RPC calls.</param>
        /// <param name="maxBackOffWaitTime">The maximum amount of time we will sleep when backing off failed RPC calls</param>
        public BreakpointActionServer(IBreakpointServer server, CancellationTokenSource cts,
            TimeSpan? minBackOffWaitTime = null, TimeSpan? maxBackOffWaitTime = null)
        {
            _cts = GaxPreconditions.CheckNotNull(cts, nameof(cts));
            _server = GaxPreconditions.CheckNotNull(server, nameof(server));
            _minBackOffWaitTime = minBackOffWaitTime ?? _defaultMinBackOffWaitTime;
            _maxBackOffWaitTime = maxBackOffWaitTime ?? _defaultMaxBackOffWaitTime;
        }

        /// <inheritdoc />
        public void Dispose() => _server.Dispose();

        /// <summary>
        /// Waits for the underlying <see cref="IBreakpointServer"/> to connect.
        /// </summary>
        public void WaitForConnection() => _server.WaitForConnectionAsync().Wait();
 
        /// <summary>
        /// The main logic of the action server. To be implemented by each parent class.
        /// </summary>
        internal abstract void MainAction();

        /// <summary>
        /// Repeats the <see cref="MainAction"/> function.  If an <see cref="RpcException"/>
        /// is thrown the <paramref name="waitTime"/> will double until the action is
        /// successful (up to <see cref="_maxBackOffWaitTime"/>).  When the action
        /// is successful the wait between calls will return to the original amount.
        /// </summary>
        /// <param name="waitTime">The time to wait between calls to the <see cref="MainAction"/>.</param>
        /// <param name="cancellationToken">A token to signal this loop should stop.</param>
        public void StartActionLoop(TimeSpan waitTime, CancellationToken cancellationToken)
        {
            TimeSpan originalWaitTime = waitTime;
            TimeSpan currentWaitTime = waitTime;
            while (!cancellationToken.IsCancellationRequested)
            {
                try
                {
                    MainAction();
                    currentWaitTime = originalWaitTime;
                }
                catch (RpcException e)
                {
                    Console.WriteLine($"RpcException with status code '{e.Status.StatusCode}' \n {e}");
                    if (currentWaitTime < _maxBackOffWaitTime)
                    {
                        currentWaitTime = TimeSpan.FromTicks(
                            Math.Min(currentWaitTime.Ticks * 2, _maxBackOffWaitTime.Ticks));
                    }
                    currentWaitTime = currentWaitTime == TimeSpan.Zero ? _minBackOffWaitTime : currentWaitTime;
                }
                Thread.Sleep(currentWaitTime);
            }
        }
    }
}
