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

using Google.Protobuf.WellKnownTypes;
using System;
using System.Linq;
using Xunit;
using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;
using StackdriverSourceLocation = Google.Cloud.Debugger.V2.SourceLocation;
using System.Collections.Generic;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class BreakpointExtensionsTests
    {
        private const string _id = "breakpoint-id";
        private const string _path = "C:\\breakpoint-Path";
        private const string _condition = "x == 2";
        private string[] _expressions =
        {
            "a", "b.c"
        };
        private const int _line = 11;

        [Fact]
        public void Convert_Breakpoint()
        {
            var sdBreakpoint = new StackdriverBreakpoint
            {
                Id = _id,
                Location = new StackdriverSourceLocation
                {
                    Path = _path,
                    Line = _line
                },
                CreateTime = Timestamp.FromDateTime(DateTime.UtcNow),
                Condition = _condition,
                Expressions = { _expressions }
            };

            var breakpoint = sdBreakpoint.Convert();
            Assert.Equal(_id, breakpoint.Id);
            Assert.Equal(_path, breakpoint.Location.Path);
            Assert.Equal(_line, breakpoint.Location.Line);
            Assert.True(breakpoint.Activated);
            Assert.Null(breakpoint.CreateTime);
            Assert.Equal(_condition, breakpoint.Condition);
            Assert.Equal(_expressions, breakpoint.Expressions);
        }

        [Fact]
        public void Convert_StackdriverBreakpoint()
        {
            var breakpoint = new Breakpoint
            {
                Id = _id,
                Location = new SourceLocation
                {
                    Path = _path,
                    Line = _line
                },
                CreateTime = Timestamp.FromDateTime(DateTime.UtcNow),
                FinalTime = Timestamp.FromDateTime(DateTime.UtcNow.AddSeconds(10)),
                StackFrames =
                {
                    new StackFrame
                    {
                        MethodName = "method-one",
                        Location = new SourceLocation
                        {
                            Path = "path",
                            Line = 10,
                        }
                    },
                    new StackFrame
                    {
                        MethodName = "method-two",
                        Location = new SourceLocation
                        {
                            Path = "path",
                            Line = 11,
                        },
                        Arguments =
                        {
                            new Variable(),
                            new Variable(),
                        },
                    }
                },
                EvaluatedExpressions =
                {
                    new Variable()
                    {
                        Name = "first-expression"
                    },
                    new Variable()
                    {
                        Name = "second-expression"
                    }
                }
            };

            var sdBreakpoint = breakpoint.Convert();
            Assert.Equal(_id, sdBreakpoint.Id);
            Assert.Equal(_path.Replace('\\', '/'), sdBreakpoint.Location.Path);
            Assert.Equal(_line, sdBreakpoint.Location.Line);
            Assert.Equal(breakpoint.CreateTime, sdBreakpoint.CreateTime);
            Assert.Equal(breakpoint.FinalTime, sdBreakpoint.FinalTime);
            Assert.Equal(2, sdBreakpoint.StackFrames.Count);
            Assert.Single(sdBreakpoint.StackFrames.Where(sf => sf.Function.Equals("method-one")));
            var sfTwo = sdBreakpoint.StackFrames.Where(sf => sf.Function.Equals("method-two"));
            Assert.Equal(2, sfTwo.Single().Arguments.Count);
            Assert.Equal(2, sdBreakpoint.EvaluatedExpressions.Count);
            Assert.Single(
                sdBreakpoint.EvaluatedExpressions.Where(ee => ee.Name.Equals("first-expression")));
            Assert.Single(
                sdBreakpoint.EvaluatedExpressions.Where(ee => ee.Name.Equals("second-expression")));
        }
    }
}
