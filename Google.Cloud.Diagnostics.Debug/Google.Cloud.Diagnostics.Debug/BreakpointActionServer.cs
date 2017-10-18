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

namespace Google.Cloud.Diagnostics.Debug
{
    public abstract class BreakpointActionServer :IDisposable
    {
        /// <summary>
        /// The minimum amount of time we will sleep when backing off failed RPC calls.
        /// The wait time may be higher if the user sets the option.
        /// </summary>
        private static readonly TimeSpan _minBackOffWaitTime = TimeSpan.FromSeconds(10);

        /// <summary>
        /// The maximum amount of time we will sleep when backing off failed RPC calls.
        /// The wait time may be higher if the user sets the option.
        /// </summary>
        private static readonly TimeSpan _maxBackOffWaitTime = TimeSpan.FromSeconds(10);

        protected readonly IBreakpointServer _server;

        public BreakpointActionServer(IBreakpointServer server)
        {
            _server = GaxPreconditions.CheckNotNull(server, nameof(server));
        }

        public void Dispose() => _server.Dispose();

        public void WaitForConnection() => _server.WaitForConnectionAsync().Wait();
 

        protected abstract void MainAction();


        /*
          /// <summary>
        /// Repeats an action that may throw and <see cref="RpcException"/>.
        /// If an <see cref="RpcException"/> is thrown the <paramref name="waitTime"/> will double
        /// until the action is successful (up to <see cref="_maxBackOffWaitTime"/>).  When the action
        /// is successful the wait between calls will return to the original amount.
        /// </summary>
        /// <param name="waitTime">The time to wait between calls to the action.</param>
        /// <param name="cancellationToken">A token to signal this action should stop.</param>
         */

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
                        currentWaitTime = TimeSpan.FromTicks(currentWaitTime.Ticks * 2);
                    }
                    currentWaitTime = currentWaitTime == TimeSpan.Zero ? _minBackOffWaitTime : currentWaitTime;
                }
                Thread.Sleep(currentWaitTime);
            }
        }
    }
}
