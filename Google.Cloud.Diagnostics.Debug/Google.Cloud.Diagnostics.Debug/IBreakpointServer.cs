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

using System;
using System.Threading;
using System.Threading.Tasks;

namespace Google.Cloud.Diagnostics.Debug
{
    public interface IBreakpointServer : IDisposable
    {
        /// <summary>
        /// Waits for a client to connect.
        /// </summary>
        Task WaitForConnectionAsync();

        /// <summary>
        /// Read a breakpoint from the client.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The breakpoint message.</returns>
        Task<Breakpoint> ReadBreakpointAsync(CancellationToken cancellationToken = default(CancellationToken));

        /// <summary>
        /// Write a breakpoint to the client.
        /// </summary>
        /// <param name="breakpoint">The breakpoint to write.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>A task representing the asynchronous operation.</returns>
        Task WriteBreakpointAsync(Breakpoint breakpoint, CancellationToken cancellationToken = default(CancellationToken));
    }
}