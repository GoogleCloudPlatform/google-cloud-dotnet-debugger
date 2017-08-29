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

using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class StackFrameExtensionsTests
    {
        private const string _method = "frame-method";
        private const string _path = "frame-path";
        private const int _line = 11;

        [Fact]
        public void Convert()
        {
            var stackframe = new StackFrame
            {
                MethodName = _method,
                Location = new SourceLocation
                {
                    Path = _path,
                    Line = _line,
                },
                Arguments =
                {
                    new Variable(),
                },
                Locals =
                {
                    new Variable(),
                    new Variable(),
                    new Variable(),
                }
            };

            var sdStackFrame = stackframe.Convert();
            Assert.Equal(_method, sdStackFrame.Function);
            Assert.Equal(_path, sdStackFrame.Location.Path);
            Assert.Equal(_line, sdStackFrame.Location.Line);
            Assert.Single(sdStackFrame.Arguments);
            Assert.Equal(3, sdStackFrame.Locals.Count);
        }

    }
}
