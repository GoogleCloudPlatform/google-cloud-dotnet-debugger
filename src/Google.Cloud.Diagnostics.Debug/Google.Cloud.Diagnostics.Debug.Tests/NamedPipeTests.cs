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
using System.IO.Pipes;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class NamedPipeTests
    {
        private readonly Mock<PipeStream> _mockStream;
        private readonly INamedPipe _pipe;

        public NamedPipeTests()
        {
            _mockStream = new Mock<PipeStream>(PipeDirection.InOut, 1024);
            _pipe = new NamedPipe(_mockStream.Object);
        }

        [Fact]
        public async Task ReadAsync()
        {
            byte[] bytesRet = Encoding.ASCII.GetBytes("Some random string");
            _mockStream.Setup(s => s.ReadAsync(
                It.IsAny<byte[]>(), 0, Constants.BufferSize, It.IsAny<CancellationToken>()))
                .Callback((byte[] arr, int offset, int count, CancellationToken token) =>
                {
                    for (int i = 0; i < bytesRet.Length; i++)
                    {
                        arr[i] = bytesRet[i];
                    }  
                })
                .Returns(Task.FromResult(bytesRet.Length));

            Assert.Equal(bytesRet, await _pipe.ReadAsync());
            _mockStream.VerifyAll();
        }

        [Fact]
        public async Task ReadAsync_Full()
        {
            var byteStr = Encoding.ASCII.GetBytes("Some random string");
            byte[] bytesRet = new byte[Constants.BufferSize];
            for (int i = 0; i < bytesRet.Length; i++)
            {
                bytesRet[i] = byteStr[i % byteStr.Length];
            }

            _mockStream.Setup(s => s.ReadAsync(
                It.IsAny<byte[]>(), 0, Constants.BufferSize, It.IsAny<CancellationToken>()))
                .Callback((byte[] arr, int offset, int count, CancellationToken token) =>
                {
                    for (int i = 0; i < bytesRet.Length; i++)
                    {
                        arr[i] = bytesRet[i];
                    }
                })
                .Returns(Task.FromResult(0));

            Assert.Equal(bytesRet, await _pipe.ReadAsync());
            _mockStream.VerifyAll();
        }

        [Fact]
        public void WriteAsync()
        {
            byte[] bytes = Encoding.ASCII.GetBytes("Some random string");
            _mockStream.Setup(s => s.WriteAsync(bytes, 0, bytes.Length, It.IsAny<CancellationToken>()))
                .Returns(Task.FromResult(true));
            _mockStream.Setup(s => s.FlushAsync(It.IsAny<CancellationToken>())).Returns(Task.FromResult(true));

            _pipe.WriteAsync(bytes);
            _mockStream.VerifyAll();
        }

        [Fact]
        public void WriteAsync_Multiple()
        {
            var byteStr = Encoding.ASCII.GetBytes("Some random string");
            byte[] bytes = new byte[Constants.BufferSize * 2 + 100];
            for (int i = 0; i < bytes.Length; i++)
            {
                bytes[i] = byteStr[i % byteStr.Length];
            }

            _mockStream.Setup(s => s.WriteAsync(bytes, 0, Constants.BufferSize, It.IsAny<CancellationToken>()))
                .Returns(Task.FromResult(true));
            _mockStream.Setup(s => 
                s.WriteAsync(bytes, Constants.BufferSize, Constants.BufferSize, It.IsAny<CancellationToken>()))
                .Returns(Task.FromResult(true));
            _mockStream.Setup(s =>
                s.WriteAsync(bytes, Constants.BufferSize * 2, 100, It.IsAny<CancellationToken>()))
                .Returns(Task.FromResult(true));
            _mockStream.Setup(s => s.FlushAsync(It.IsAny<CancellationToken>())).Returns(Task.FromResult(true));

            _pipe.WriteAsync(bytes);
            _mockStream.VerifyAll();
        }
    }
}
