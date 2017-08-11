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
using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;
using StackdriverSourceLocation = Google.Cloud.Debugger.V2.SourceLocation;
using StackdriverStackFrame = Google.Cloud.Debugger.V2.StackFrame;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// Extensions for <see cref="Breakpoint"/>s and <see cref="StackdriverBreakpoint"/>s.
    /// </summary>
    internal static class BreakpointExtensions
    {
        /// <summary>
        /// Converts a <see cref="StackdriverBreakpoint"/> to a <see cref="Breakpoint"/>.
        /// </summary>
        public static Breakpoint Convert(this StackdriverBreakpoint breakpoint)
        {
            GaxPreconditions.CheckNotNull(breakpoint, nameof(breakpoint));
            return new Breakpoint
            {
                Id = breakpoint.Id,
                Activated = true,
                Location = new Google.Cloud.Diagnostics.Debug.SourceLocation
                {
                    Line = breakpoint.Location?.Line ?? 0,
                    Path = breakpoint.Location?.Path,
                },
            };
        }

        /// <summary>
        /// Converts a <see cref="Breakpoint"/> to a <see cref="StackdriverBreakpoint"/>.
        /// </summary>
        public static StackdriverBreakpoint Convert(this Breakpoint breakpoint)
        {
            GaxPreconditions.CheckNotNull(breakpoint, nameof(breakpoint));
            return new StackdriverBreakpoint
            {
                CreateTime = breakpoint.CreateTime,
                FinalTime = breakpoint.FinalTime, // TODO(talarico): Is this set properly before?
                Id = breakpoint.Id,
                Location = new StackdriverSourceLocation
                {
                    Path = breakpoint.Location?.Path,
                    Line = breakpoint.Location?.Line ?? 0
                },

                // TODO(talarico): we need more than one stackframe.
                StackFrames = 
                {
                    new StackdriverStackFrame {
                        Function = breakpoint.MethodName,
                        Locals = { breakpoint.Variables?.Select(x => x.Convert()).ToList() },
                        Location = new StackdriverSourceLocation
                        {
                            Path = breakpoint.Location?.Path,
                            Line = breakpoint.Location?.Line ?? 0
                        },
                    },
                }
            };
        }
    }
}
