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
    public sealed class BreakpointManager
    {
        public class BreakpointManagerResponse
        {
            public IEnumerable<StackdriverBreakpoint> New { get; set; }
            public IEnumerable<StackdriverBreakpoint> Removed { get; set; }
        }

        private readonly Dictionary<string, StackdriverBreakpoint> _breakpointLocationToId =
            new Dictionary<string, StackdriverBreakpoint>();

        private readonly object _mutex = new object();

        public BreakpointManagerResponse UpdateBreakpoints(IEnumerable<StackdriverBreakpoint> activeBreakpoints)
        {
            lock (_mutex)
            {
                var identifiersToBreakpoint = new Dictionary<string, StackdriverBreakpoint>(
                    activeBreakpoints.ToDictionary(b => b.GetLocationIdentifier(), b => b));

                var newBreakpoints = activeBreakpoints.Where(b => !_breakpointLocationToId.ContainsKey(b.GetLocationIdentifier())).ToList();
                var removedBreakpoints = _breakpointLocationToId.Keys
                    .Where(identifier => !identifiersToBreakpoint.ContainsKey(identifier))
                    .Select(id => _breakpointLocationToId[id])
                    .ToList();

                foreach (var newBreakpoint in newBreakpoints)
                {
                    _breakpointLocationToId[newBreakpoint.GetLocationIdentifier()] = newBreakpoint;
                }

                foreach (var removedBreakpoint in removedBreakpoints)
                {
                    _breakpointLocationToId.Remove(removedBreakpoint.GetLocationIdentifier());
                }

                return new BreakpointManagerResponse
                {
                    New = newBreakpoints,
                    Removed = removedBreakpoints
                };
            }
        }
    }
}