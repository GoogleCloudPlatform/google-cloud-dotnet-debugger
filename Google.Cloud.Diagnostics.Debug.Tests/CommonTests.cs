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

using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class CommonTests
    {
        [Fact]
        public void CreateStatusMessage()
        {
            var message = "This is a test message!";

            var statusMessage = Common.CreateStatusMessage(message);
            Assert.Equal(message, statusMessage.Description.Format);
            Assert.False(statusMessage.IsError);

            var statusMessageError = Common.CreateStatusMessage(message, true);
            Assert.Equal(message, statusMessageError.Description.Format);
            Assert.True(statusMessageError.IsError);
        }
    }
}
