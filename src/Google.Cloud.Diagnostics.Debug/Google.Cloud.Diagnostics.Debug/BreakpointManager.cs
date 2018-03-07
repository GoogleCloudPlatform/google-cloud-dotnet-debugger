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

using System.Collections.Generic;
using System.Linq;
using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// Manages the current state of breakpoints.
    /// </summary>
    public sealed class BreakpointManager
    {
        /// <summary>
        /// A response from the <see cref="BreakpointManager"/>
        /// </summary>
        public class BreakpointManagerResponse
        {
            /// <summary>
            /// New breakpoints from the last update.
            /// </summary>
            public IEnumerable<StackdriverBreakpoint> New { get; set; }

            /// <summary>
            /// Breakpoints from during this update.
            /// </summary>
            public IEnumerable<StackdriverBreakpoint> Removed { get; set; }
        }

        /// <summary>
        /// The list of current breakpoints.
        /// </summary>
        private readonly Dictionary<string, StackdriverBreakpoint> _breakpointDictionary =
            new Dictionary<string, StackdriverBreakpoint>();

        /// <summary>A lock to protect the breakpoint dictionary.</summary>
        private readonly object _mutex = new object();

        /// <summary>
        /// Update the current set of active breakpoints.
        /// </summary>
        /// <param name="activeBreakpoints">The current set of active breakpoints from the debugger API.</param>
        public BreakpointManagerResponse UpdateBreakpoints(IEnumerable<StackdriverBreakpoint> activeBreakpoints)
        {
            lock (_mutex)
            {
                var identifiersToBreakpoint = new Dictionary<string, StackdriverBreakpoint>();
                foreach (var breakpoint in activeBreakpoints)
                {
                    if (!identifiersToBreakpoint.ContainsKey(breakpoint.Id))
                    {
                        identifiersToBreakpoint.Add(breakpoint.Id, breakpoint);
                    }
                }

                var newBreakpoints = identifiersToBreakpoint.Values.Where(
                    b => !_breakpointDictionary.ContainsKey(b.Id)).ToList();
                foreach (var newBreakpoint in newBreakpoints)
                {
                    _breakpointDictionary[newBreakpoint.Id] = newBreakpoint;
                }

                var removedBreakpoints = _breakpointDictionary.Keys
                    .Where(identifier => !identifiersToBreakpoint.ContainsKey(identifier))
                    .Select(id => _breakpointDictionary[id])
                    .ToList();
                foreach (var removedBreakpoint in removedBreakpoints)
                {
                    _breakpointDictionary.Remove(removedBreakpoint.Id);
                }

                return new BreakpointManagerResponse
                {
                    New = newBreakpoints,
                    Removed = removedBreakpoints
                };
            }
        }

        /// <summary>
        /// Gets the breakpoint id from the manager based on the breakpoint location.
        /// If the list does not contain
        /// Note: This is needed as the debugger does not pay attention to breakpoint ids
        /// and must be managed here.
        /// </summary>
        /// <param name="breakpoint">The breakpoint to get the id of.</param>
        /// <returns>The breakpoint id or null if none can be found.</returns>
        public string GetBreakpointId(StackdriverBreakpoint breakpoint)
        {
            StackdriverBreakpoint bp;
            return _breakpointDictionary.TryGetValue(
                breakpoint.Id, out bp) ? bp.Id : null;
        }
    }
}
