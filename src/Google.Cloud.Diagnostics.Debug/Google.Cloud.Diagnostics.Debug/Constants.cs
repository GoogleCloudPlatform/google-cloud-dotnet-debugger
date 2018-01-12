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

using System.Text;

namespace Google.Cloud.Diagnostics.Debug
{
    public static class Constants
    {
        /// <summary>The buffer size to read and write pipes.</summary>
        public const int BufferSize = 1024;

        /// <summary>The name of the named pipe.</summary>
        public const string PipeName = "dotnet-debugger";

        /// <summary>The start of a breakpoint message.</summary>
        public static readonly byte[] StartBreakpointMessage = Encoding.ASCII.GetBytes("START_DEBUG_MESSAGE");

        /// <summary>The end of a breakpoint message.</summary>
        public static readonly byte[] EndBreakpointMessage = Encoding.ASCII.GetBytes("END_DEBUG_MESSAGE");
    }
}
