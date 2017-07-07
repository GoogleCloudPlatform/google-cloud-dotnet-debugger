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
using System.IO.Pipes;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// A named pipe server.
    /// </summary>
    public class NamedPipeServer : INamedPipe, IDisposable
    {
        private readonly NamedPipeServerStream _server;

        public NamedPipeServer()
        {
            _server = new NamedPipeServerStream(
               pipeName: Constants.PipeName,
               direction: PipeDirection.InOut,
               // TODO(talarico): This should be NamedPipeServerStream.MaxAllowedServerInstance but it's not public.
               maxNumberOfServerInstances: -1,
               transmissionMode: PipeTransmissionMode.Byte,
               options: PipeOptions.Asynchronous);
        }

        /// <inheritdoc />
        public Task WaitForConnectionAsync(CancellationToken cancellationToken = default(CancellationToken)) =>
            _server.WaitForConnectionAsync(cancellationToken);

        /// <inheritdoc />
        public async Task<byte[]> ReadAsync(CancellationToken cancellationToken = default(CancellationToken))
        {
            byte[] bytes = new byte[Constants.BufferSize];
            int read = await _server.ReadAsync(bytes, 0, Constants.BufferSize, cancellationToken);
            return bytes.Take(read == 0 ? Constants.BufferSize : read).ToArray();
        }

        /// <inheritdoc />
        public async Task WriteAsync(byte[] bytes, CancellationToken cancellationToken = default(CancellationToken))
        {
            int offset = 0;
            int bytesLeft = bytes.Length;
            while (bytesLeft > 0)
            {
                int write = bytesLeft > Constants.BufferSize ? Constants.BufferSize : bytesLeft;
                await _server.WriteAsync(bytes, offset, write, cancellationToken);
                bytesLeft -= write;
                offset += write;
            }
            _server.Flush();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            if (_server.IsConnected)
            {
                _server.Disconnect();
            }
            _server.Dispose();
        }
    }
}
