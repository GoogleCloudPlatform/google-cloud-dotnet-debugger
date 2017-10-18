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

using System.Linq;
using System.Net.Http;
using System.Threading.Tasks;
using Xunit;
using DebuggerVariable = Google.Cloud.Debugger.V2.Variable;
using DebuggerStackFrame = Google.Cloud.Debugger.V2.StackFrame;

namespace Google.Cloud.Diagnostics.Debug.IntegrationTests
{
    public class DebuggerTests : DebuggerTestBase
    {
        public DebuggerTests() : base() { }

        [Fact]
        public async Task BreakpointHit()
        {
            using (StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(Module, Version);
                var breakpoint = SetBreakpoint(debuggee.Id, "MainController.cs", 26);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(AppUrlBase);
                }

                var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                // Check that the breakpoint has been hit.
                Assert.True(newBp.IsFinalState);
            }           
        }

        /// <summary>
        /// Calls to the endpoint AppUrlEcho, and sets a breakpoint just
        /// before it returns. At this breakpoint, we can collect and examine
        /// List, HashSet and Dictionary collection.
        /// </summary>
        [Fact]
        public async Task TestCollection()
        {
            using (StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(Module, Version);
                var breakpoint = SetBreakpoint(debuggee.Id, "MainController.cs", 42);
                string collectionKey = "RandomKey";

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync($"{AppUrlEcho}/{collectionKey}");
                }

                var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                // Check that the breakpoint has been hit.
                Assert.True(newBp.IsFinalState);

                // Checks that breakpoint contains collection.
                DebuggerStackFrame firstFrame = newBp.StackFrames[0];

                TestCollectionHelper("List", collectionKey, firstFrame);
                TestCollectionHelper("Set", collectionKey, firstFrame);
                TestCollectionHelper("Dictionary", collectionKey, firstFrame);
            }
        }

        private void TestCollectionHelper(string collectionName, string collectionKey, DebuggerStackFrame stackFrame)
        {
            // Tests that list contains the correct item.
            DebuggerVariable collection = stackFrame.Locals.FirstOrDefault(localVar => localVar.Name == $"test{collectionName}");
            Assert.NotNull(collection);

            Assert.Equal(6, collection.Members.Count);
            DebuggerVariable collectionCount = collection.Members.FirstOrDefault(member => member.Name == "Count");
            Assert.NotNull(collectionCount);
            Assert.Equal("5", collectionCount.Value);
            for (int i = 0; i < 5; i += 1)
            {
                DebuggerVariable item = collection.Members.FirstOrDefault(member => member.Name == $"[{i}]");
                Assert.NotNull(item);
                if (collectionName == "Dictionary")
                {
                    DebuggerVariable key = item.Members.FirstOrDefault(member => member.Name == "key");
                    DebuggerVariable value = item.Members.FirstOrDefault(member => member.Name == "value");
                    Assert.NotNull(key);
                    Assert.NotNull(value);
                    Assert.Equal($"Key{collectionKey}{i}", key.Value);
                    Assert.Equal($"{i}", value.Value);
                }
                else
                {
                    Assert.Equal($"{collectionName}{collectionKey}{i}", item.Value);
                }
            }
        }
    }
}
