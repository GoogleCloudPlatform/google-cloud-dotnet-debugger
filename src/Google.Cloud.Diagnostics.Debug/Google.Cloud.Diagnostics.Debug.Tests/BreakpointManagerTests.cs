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

using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;
using Xunit;

using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class BreakpointManagerTests
    {
        private static readonly ImmutableList<StackdriverBreakpoint> _emptyList =
            ImmutableList<StackdriverBreakpoint>.Empty;
        private readonly BreakpointManager _manager;

        public BreakpointManagerTests()
        {
            _manager = new BreakpointManager();
        }

        [Fact]
        public void UpdateBreakpoints_NewBreakpoints()
        {
            var breakpoints = CreateBreakpoints(2);
            var response = _manager.UpdateBreakpoints(breakpoints);

            Assert.Empty(response.Removed);
            Assert.Equal(2, response.New.Count());

            Assert.Single(response.New.Where(b => b.Id == breakpoints[0].Id));
            Assert.Single(response.New.Where(b => b.Id == breakpoints[1].Id));
        }

        [Fact]
        public void UpdateBreakpoints_Duplicate()
        {
            var breakpoints = CreateBreakpoints(1).Concat(CreateBreakpoints(1)).ToList();
            var response = _manager.UpdateBreakpoints(breakpoints);

            Assert.Empty(response.Removed);
            Assert.Single(response.New);

            Assert.Equal(breakpoints[0], response.New.Single());
        }

        [Fact]
        public void UpdateBreakpoints_IgnoreDuplicateBreakpoints()
        {
            var breakpoints = CreateBreakpoints(2);
            var response = _manager.UpdateBreakpoints(breakpoints);

            Assert.Equal(2, response.New.Count());

            response = _manager.UpdateBreakpoints(breakpoints);

            Assert.Empty(response.New);
            Assert.Empty(response.Removed);
        }

        [Fact]
        public void UpdateBreakpoints_IgnoreDuplicateBreakpointsBySource()
        {
            var breakpoints = CreateBreakpoints(2);
            var response = _manager.UpdateBreakpoints(breakpoints);

            Assert.Equal(2, response.New.Count());

            foreach (var breakpoint in breakpoints)
            {
                breakpoint.Id = "" + (Int32.Parse(breakpoint.Id) * 2);
            }

            response = _manager.UpdateBreakpoints(breakpoints);

            Assert.Empty(response.New);
            Assert.Empty(response.Removed);
        }

        [Fact]
        public void UpdateBreakpoints_RemoveBreakpoints()
        {
            var breakpoints = CreateBreakpoints(2);
            _manager.UpdateBreakpoints(breakpoints);
            var response = _manager.UpdateBreakpoints(CreateBreakpoints(1));

            Assert.Empty(response.New);
            Assert.Equal(breakpoints[1].Id, response.Removed.Single().Id);
        }

        [Fact]
        public void UpdateBreakpoints_AddAndRemoveBreakpoints()
        {
            var breakpoints = CreateBreakpoints(10);
            var response = _manager.UpdateBreakpoints(breakpoints.GetRange(0, 6));
            Assert.Equal(6, response.New.Count());
            Assert.Empty(response.Removed);

            response = _manager.UpdateBreakpoints(breakpoints.GetRange(1, 7));
            Assert.Equal(2, response.New.Count());
            Assert.Single(response.Removed);

            response = _manager.UpdateBreakpoints(breakpoints.GetRange(4, 5));
            Assert.Single(response.New);
            Assert.Equal(3, response.Removed.Count());

            response = _manager.UpdateBreakpoints(new List<StackdriverBreakpoint>());
            Assert.Empty(response.New);
            Assert.Equal(5, response.Removed.Count());
        }

        /// <summary>
        /// Create a list of <see cref="StackdriverBreakpoint"/>s.
        /// </summary>
        private List<StackdriverBreakpoint> CreateBreakpoints(int numBreakpoints)
        {
            var breakpoints = new List<StackdriverBreakpoint>();
            for (int i = 0; i < numBreakpoints; i++)
            {
                breakpoints.Add(new StackdriverBreakpoint
                {
                    Id = "" + i,
                    Location = new Debugger.V2.SourceLocation
                    {
                        Path = "" + i + i,
                        Line = i,
                    }
                });
            }
            return breakpoints;
        }
    }
}
