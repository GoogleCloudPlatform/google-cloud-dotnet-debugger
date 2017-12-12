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

using Google.Api.Gax;
using System.IO.Pipes;
using System.Threading;
using System.Threading.Tasks;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// A named pipe server.
    /// </summary>
    public class NamedPipeServer : INamedPipeServer
    {
        private readonly INamedPipe _pipe;
        private readonly NamedPipeServerStream _server;

        /// <summary>
        /// Create a new <see cref="NamedPipeServer"/>.
        /// </summary>
        /// <param name="pipeName">The name of the pipe.</param>
        public NamedPipeServer(string pipeName)
        {
            _server = new NamedPipeServerStream(
               pipeName: GaxPreconditions.CheckNotNullOrEmpty(pipeName, nameof(pipeName)),
               direction: PipeDirection.InOut,
               // TODO(talarico): This should be NamedPipeServerStream.MaxAllowedServerInstance but it's not public.
               maxNumberOfServerInstances: -1,
               transmissionMode: PipeTransmissionMode.Byte,
               options: PipeOptions.Asynchronous);
            _pipe = new NamedPipe(_server);
        }

        /// <inheritdoc />
        public Task WaitForConnectionAsync(CancellationToken cancellationToken = default(CancellationToken))
            => _server.WaitForConnectionAsync(cancellationToken);

        /// <inheritdoc />
        public Task<byte[]> ReadAsync(CancellationToken cancellationToken = default(CancellationToken))
            => _pipe.ReadAsync(cancellationToken);

        /// <inheritdoc />
        public Task WriteAsync(byte[] bytes, CancellationToken cancellationToken = default(CancellationToken))
            => _pipe.WriteAsync(bytes, cancellationToken);

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
