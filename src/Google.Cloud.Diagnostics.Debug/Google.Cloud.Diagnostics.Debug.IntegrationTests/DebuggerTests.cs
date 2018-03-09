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
using DebuggerBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;

namespace Google.Cloud.Diagnostics.Debug.IntegrationTests
{
    public class DebuggerTests : DebuggerTestBase
    {
        public DebuggerTests() : base() { }

        [Fact]
        public async Task BreakpointHit()
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.HelloLine);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(app.AppUrlBase);
                }

                var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                // Check that the breakpoint has been hit.
                Assert.True(newBp.IsFinalState);
            }           
        }

         [Fact]
        public async Task BreakpointHit_Details()
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.EchoBottomLine);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync($"{app.AppUrlEcho}/1");
                }

                var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                // Check basic breakpoint values.
                Assert.True(newBp.IsFinalState);
                Assert.Equal(DebuggerBreakpoint.Types.Action.Capture, newBp.Action);
                Assert.Equal(breakpoint.CreateTime, newBp.CreateTime);
                Assert.Empty(newBp.Expressions);
                Assert.Empty(newBp.Condition);
                Assert.True(newBp.FinalTime.ToDateTime() > newBp.CreateTime.ToDateTime());

                // Check the stack frames
                Assert.Equal(20, newBp.StackFrames.Count);

                // Only check the first one as it's the only one with information we care about. 
                var stackframe = newBp.StackFrames[0];

                // Check the function is echo.
                Assert.Contains($"{typeof(TestApp.MainController).ToString()}.Echo", stackframe.Function);

                // Check the location is accurate.
                var location = stackframe.Location;
                Assert.Equal(TestApplication.EchoBottomLine, location.Line);
                Assert.Contains("src/Google.Cloud.Diagnostics.Debug/Google.Cloud.Diagnostics.Debug.TestApp/MainController.cs", location.Path);

                // Two arguments, the param 'message' and 'this'.
                var arguments = stackframe.Arguments;
                Assert.Equal(2, arguments.Count);

                // Check 'this' arg for the class values.
                var thisArg = arguments.Where(l => l.Name == "this").Single();
                Assert.Equal(typeof(TestApp.MainController).ToString(), thisArg.Type);
                Assert.Equal(2, thisArg.Members.Count);

                var privateStaticString = thisArg.Members.Where(m => m.Name == "_privateReadOnlyString").Single();
                Assert.Equal(typeof(System.String).ToString(), privateStaticString.Type);

                var publicString = thisArg.Members.Where(m => m.Name == "publicString").Single();
                Assert.Equal(typeof(System.String).ToString(), publicString.Type);
                Assert.Equal(new TestApp.MainController().publicString, publicString.Value);

                // Check 'message' arg.
                var messageArg = arguments.Where(l => l.Name == "message").Single();
                Assert.Equal(typeof(System.String).ToString(), messageArg.Type);
                Assert.Equal("1", messageArg.Value);

                // The list, set and dictionary are tested below.
                var locals = stackframe.Locals;

                // One of the local variables will be the 'i' from the for loop
                // but will have a name like value_*.
                var iLocal = locals.Where(m => m.Value == "5").Single();
                Assert.Equal(typeof(System.Int32).ToString(), iLocal.Type);

                // One of the local variables will be the condition for the for loop
                // but will have a name like value_*.
                var condLocal = locals.Where(m => m.Value == "0").Single();
                Assert.Equal(typeof(System.Boolean).ToString(), condLocal.Type);

                // One of the local variables will be the function's return value
                // but will have a name like value_*.
                var returnLocal = locals.Where(m => m.Value == "1").Single();
                Assert.Equal(typeof(System.String).ToString(), returnLocal.Type);
            }           
        }

        [Fact]
        public async Task BreakpointHit_Reset()
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.HelloLine);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(app.AppUrlBase);
                    var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);
                    Assert.True(newBp.IsFinalState);

                    var breakpoint2 = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.HelloLine);
                    await client.GetAsync(app.AppUrlBase);
                    var newBp2 = Polling.GetBreakpoint(debuggee.Id, breakpoint2.Id);
                    Assert.True(newBp2.IsFinalState);
                }
            }
        }

        [Fact]
        public async Task BreakpointSet_TwoSameLocation()
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint = SetBreakpoint(debuggee.Id, TestApplication.MainClass, TestApplication.HelloLine);
                var breakpoint2 = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.HelloLine);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(app.AppUrlBase);
                    var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);
                    Assert.True(newBp.IsFinalState);

                    await client.GetAsync(app.AppUrlBase);
                    var newBp2 = Polling.GetBreakpoint(debuggee.Id, breakpoint2.Id);
                    Assert.True(newBp2.IsFinalState);
                }
            }
        }

        [Fact]
        public async Task BreakpointsSet_Multiple()
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint1 = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.HelloLine);
                var breakpoint2 = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.EchoBottomLine);
                var breakpoint3 = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.PidLine);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync($"{app.AppUrlEcho}/1");
                    var newBp2 = Polling.GetBreakpoint(debuggee.Id, breakpoint2.Id);
                    Assert.True(newBp2.IsFinalState);

                    await client.GetAsync(app.AppUrlBase);
                    var newBp1 = Polling.GetBreakpoint(debuggee.Id, breakpoint1.Id);
                    Assert.True(newBp1.IsFinalState);
                    
                    await client.GetAsync(app.AppUrlProcessId);
                    var newBp3 = Polling.GetBreakpoint(debuggee.Id, breakpoint3.Id);
                    Assert.True(newBp3.IsFinalState);
                }
            }
        }

        /// <summary>
        /// Calls to the endpoint AppUrlEcho, and sets a breakpoint just
        /// before it returns. At this breakpoint, we can collect and examine
        /// List, HashSet and Dictionary collection.
        /// </summary>
        [Theory]
        [InlineData("List")]
        [InlineData("Set")]
        [InlineData("Dictionary")]
        public async Task TestCollection(string collectionName)
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.EchoBottomLine);
                string collectionKey = "RandomKey";

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync($"{app.AppUrlEcho}/{collectionKey}");
                }

                var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                // Check that the breakpoint has been hit.
                Assert.True(newBp.IsFinalState);

                // Checks that the first frame of the breakpoint contains collection.
                DebuggerVariable collection = newBp.StackFrames[0].Locals.FirstOrDefault(localVar
                    => localVar.Name == $"test{collectionName}");
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
}
