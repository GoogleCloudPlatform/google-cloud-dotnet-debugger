// Copyright 2018 Google Inc. All Rights Reserved.
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

using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;
using Google.Cloud.Logging.V2;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// Interface for formatting and writing logs.
    /// </summary>
    public interface ILoggingClient
    {
        /// <summary>
        /// Formats and writes the log message in breakpoint to the
        /// Stackdriver Logging API.
        /// </summary>
        /// <returns>WriteLogEntriesResponse from the API.</returns>
        WriteLogEntriesResponse WriteLogEntry(StackdriverBreakpoint breakpoint);
    }
}
