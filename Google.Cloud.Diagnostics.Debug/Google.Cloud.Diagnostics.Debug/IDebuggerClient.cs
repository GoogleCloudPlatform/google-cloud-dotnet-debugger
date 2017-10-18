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

using Google.Protobuf;
using System.Collections.Generic;

using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;

namespace Google.Cloud.Diagnostics.Debug
{
    public interface IDebuggerClient
    {
        /// <summary>
        /// Register the current debuggee.
        /// </summary>
        /// <exception cref="DebuggeeDisabledException">If the debuggee should be disabled.</exception>
        void Register();

        /// <summary>
        /// Get a list of active breakpoints.
        /// </summary>
        /// <exception cref="DebuggeeDisabledException">If the debuggee should be disabled.</exception>
        IEnumerable<StackdriverBreakpoint> ListBreakpoints();

        /// <summary>
        /// Update a <see cref="StackdriverBreakpoint"/>.
        /// </summary>
        /// <exception cref="DebuggeeDisabledException">If the debuggee should be disabled.</exception>
        IMessage UpdateBreakpoint(StackdriverBreakpoint breakpoint);
    }
}