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
    /// <summary>
    /// Functionality of a named pipe.
    /// </summary>
    public interface INamedPipe : IDisposable
    {
        /// <summary>
        /// Waits for a pipe to connect.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>A task representing the asynchronous operation.</returns>
        Task WaitForConnectionAsync(CancellationToken cancellationToken = default(CancellationToken));

        /// <summary>
        /// Reads up to the maximum buffer size from the pipe.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The bytes of the message.</returns>
        Task<byte[]> ReadAsync(CancellationToken cancellationToken = default(CancellationToken));

        /// <summary>
        /// Writes a message to the connected pipe.
        /// </summary>
        /// <param name="bytes">The message to write.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>A task representing the asynchronous operation.</returns>
        Task WriteAsync(byte[] bytes, CancellationToken cancellationToken = default(CancellationToken));
    }
}
