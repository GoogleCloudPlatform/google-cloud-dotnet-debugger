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
                Debugger.V2.StackFrame firstFrame = newBp.StackFrames[0];

                // Tests that list contains the correct item.
                Debugger.V2.Variable list = firstFrame.Locals.FirstOrDefault(localVar => localVar.Name == "testList");
                Assert.NotNull(list);

                Assert.Equal(6, list.Members.Count);
                Debugger.V2.Variable listCount = list.Members.FirstOrDefault(member => member.Name == "Count");
                Assert.NotNull(listCount);
                Assert.Equal("5", listCount.Value);
                for (int i = 0; i < 5; i += 1)
                {
                    Debugger.V2.Variable item = list.Members.FirstOrDefault(member => member.Name == $"[{i}]");
                    Assert.NotNull(item);
                    Assert.Equal($"List{collectionKey}{i}", item.Value);
                }

                // Tests that set contains the correct item.
                Debugger.V2.Variable set = firstFrame.Locals.FirstOrDefault(localVar => localVar.Name == "testSet");
                Assert.NotNull(set);

                Assert.Equal(6, set.Members.Count);
                Debugger.V2.Variable setCount = set.Members.FirstOrDefault(member => member.Name == "Count");
                Assert.NotNull(setCount);
                Assert.Equal("5", setCount.Value);
                for (int i = 0; i < 5; i += 1)
                {
                    Debugger.V2.Variable item = set.Members.FirstOrDefault(member => member.Name == $"[{i}]");
                    Assert.NotNull(item);
                    Assert.Equal($"Set{collectionKey}{i}", item.Value);
                }

                // Tests that dictionary contains the correct item.
                Debugger.V2.Variable dictionary = firstFrame.Locals.FirstOrDefault(localVar => localVar.Name == "testDictionary");
                Assert.NotNull(dictionary);

                Assert.Equal(6, dictionary.Members.Count);
                Debugger.V2.Variable dictionaryCount = dictionary.Members.FirstOrDefault(member => member.Name == "Count");
                Assert.NotNull(dictionaryCount);
                Assert.Equal("5", dictionaryCount.Value);
                for (int i = 0; i < 5; i += 1)
                {
                    Debugger.V2.Variable item = dictionary.Members.FirstOrDefault(member => member.Name == $"[{i}]");
                    Assert.NotNull(item);
                    Debugger.V2.Variable key = item.Members.FirstOrDefault(member => member.Name == "key");
                    Debugger.V2.Variable value = item.Members.FirstOrDefault(member => member.Name == "value");
                    Assert.NotNull(key);
                    Assert.NotNull(value);
                    Assert.Equal($"Key{collectionKey}{i}", key.Value);
                    Assert.Equal($"{i}", value.Value);
                }
            }
        }
    }
}
