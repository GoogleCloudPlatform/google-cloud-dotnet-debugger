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
using Google.Cloud.Debugger.V2;

namespace Google.Cloud.Diagnostics.Debug
{
    internal static class Common
    {
        /// <summary>Information about the current platform.</summary>
        internal static Platform Platform = Platform.Instance();

        /// <summary>
        /// Create a <see cref="StatusMessage"/>.
        /// </summary>
        /// <param name="message">The status message.</param>
        /// <param name="isError">Optional.  True if the message is an error, defaults to false.</param>
        internal static StatusMessage CreateStatusMessage(string message, bool isError = false)
        {
            return new StatusMessage
            {
                Description = new FormatMessage
                {
                     Format = message
                },
                IsError = isError,
            };
        }
    }
}
