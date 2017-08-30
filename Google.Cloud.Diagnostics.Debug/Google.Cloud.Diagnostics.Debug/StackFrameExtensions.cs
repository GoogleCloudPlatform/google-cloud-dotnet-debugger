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
using System.Linq;
using StackdriverSourceLocation = Google.Cloud.Debugger.V2.SourceLocation;
using StackdriverStackFrame = Google.Cloud.Debugger.V2.StackFrame;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// Extensions for <see cref="StackFrame"/>s and <see cref="StackdriverStackFrame"/>s.
    /// </summary>
    internal static class StackFrameExtensions
    {
        /// <summary>
        /// Converts a <see cref="StackFrame"/> to a <see cref="StackdriverStackFrame"/>.
        /// </summary>
        public static StackdriverStackFrame Convert(this StackFrame stackframe)
        {
            GaxPreconditions.CheckNotNull(stackframe, nameof(stackframe));
            return new StackdriverStackFrame
            {
                Location = new StackdriverSourceLocation
                {
                    Path = stackframe.Location?.Path,
                    Line = stackframe.Location?.Line ?? 0
                },
                
                Function = stackframe.MethodName,
                Arguments = { stackframe.Arguments?.Select(arg => arg.Convert()).ToList() },
                Locals = { stackframe.Locals?.Select(localVar => localVar.Convert()).ToList() }
            };
        }
    }
}

