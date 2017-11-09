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
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class AgentOptionsTests
    {

        [Fact]
        public void GetDebugger()
        {
            try
            {
                var debugger = "some-path/to/a/debugger";
                Environment.SetEnvironmentVariable(AgentOptions.DebuggerEnvironmentVariable, debugger, EnvironmentVariableTarget.Process);
                Assert.Equal(debugger, AgentOptions.GetDebugger());
            }
            finally
            {
                Environment.SetEnvironmentVariable(AgentOptions.DebuggerEnvironmentVariable, null, EnvironmentVariableTarget.Process);
            }
        }

        [Fact]
        public void GetDebugger_Null() => Assert.Null(AgentOptions.GetDebugger());
    }
}
