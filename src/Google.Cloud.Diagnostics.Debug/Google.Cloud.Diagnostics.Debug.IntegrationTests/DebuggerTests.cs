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
using System.Threading;
using System;
using Google.Cloud.Debugger.V2;

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

                var publicString = thisArg.Members.Where(m => m.Name == "PublicString").Single();
                Assert.Equal(typeof(System.String).ToString(), publicString.Type);
                Assert.Equal(new TestApp.MainController().PublicString, publicString.Value);

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
        public async Task BreakpointHit_Constant()
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass,
                    TestApplication.ConstantBottomLine);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync($"{app.AppConstant}");
                }

                var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                Assert.True(newBp.IsFinalState);

                // Only check the first one as it's the only one with information we care about. 
                var stackframe = newBp.StackFrames[0];

                // Get 'this' arg for the class values so we can check the constant fields.
                DebuggerVariable thisArg = stackframe.Arguments.Where(l => l.Name == "this").Single();
                Assert.Equal(typeof(TestApp.MainController).ToString(), thisArg.Type);

                // Check the constant fields.
                var fields = thisArg.Members;

                // Check constant int field.
                DebuggerVariable constantIntField = fields.Where(m => m.Name == "ConstantInt").Single();
                Assert.Equal(typeof(System.Int32).ToString(), constantIntField.Type);
                Assert.Equal("10", constantIntField.Value);

                // Check the constant string field.
                DebuggerVariable constantStringField = fields.Where(m => m.Name == "ConstantString").Single();
                Assert.Equal(typeof(System.String).ToString(), constantStringField.Type);
                Assert.Equal("ConstantStringField", constantStringField.Value);

                // Check the constant enum field.
                DebuggerVariable constantEnumField = fields.Where(m => m.Name == "Tuesday").Single();
                Assert.Equal(typeof(DayOfWeek).ToString(), constantEnumField.Type);
                Assert.Equal("Tuesday", constantEnumField.Value);

                // Check the constant variables.
                var locals = stackframe.Locals;

                // Check the constant integer.
                DebuggerVariable constantIntVar = locals.Where(m => m.Name == "constInt").Single();
                Assert.Equal(typeof(System.Int32).ToString(), constantIntVar.Type);
                Assert.Equal("5", constantIntVar.Value);

                // Check the constant double.
                DebuggerVariable constantDoubleVar = locals.Where(m => m.Name == "constDouble").Single();
                Assert.Equal(typeof(System.Double).ToString(), constantDoubleVar.Type);
                Assert.Equal("3.500000", constantDoubleVar.Value);

                // Check the constant string.
                DebuggerVariable constantStringVar = locals.Where(m => m.Name == "constString").Single();
                Assert.Equal(typeof(System.String).ToString(), constantStringVar.Type);
                Assert.Equal("ConstString", constantStringVar.Value);

                // Check the constant enum.
                DebuggerVariable constantEnumVar = locals.Where(m => m.Name == "constEnum").Single();
                Assert.Equal(typeof(DayOfWeek).ToString(), constantEnumVar.Type);
                Assert.Equal("Monday", constantEnumVar.Value);
            }
        }

        [Fact]
        public async Task BreakpointHit_Condition()
        {
            int targetIValue = 10;
            string condition = $"i == {targetIValue}";
            using (var app = StartTestApp(debugEnabled: true))
            {
                Debuggee debuggee = Polling.GetDebuggee(app.Module, app.Version);
                DebuggerBreakpoint breakpoint = SetBreakpointAndSleep(
                    debuggee.Id, TestApplication.MainClass,
                    TestApplication.LoopMiddle, condition);

                // Checks that the breakpoint has a condition.
                Assert.Equal(breakpoint.Condition, condition);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(TestApplication.GetLoopUrl(app, 10));
                }

                DebuggerBreakpoint newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                // Check that the breakpoint has been hit.
                Assert.True(newBp.IsFinalState);

                // Check that "i" is 10 when the breakpoint is hit.
                Debugger.V2.StackFrame firstFrame = newBp.StackFrames[0];
                DebuggerVariable iVariable = firstFrame.Locals.First(local => local.Name == "i");
                Assert.Equal(iVariable.Value, targetIValue.ToString());
            }
        }

        [Fact]
        public async Task BreakpointHit_FunctionEvaluation()
        {
            string condition = "Hello() == \"Hello, World!\"";
            using (var app = StartTestApp(debugEnabled: true, methodEvaluation: true))
            {
                Debuggee debuggee = Polling.GetDebuggee(app.Module, app.Version);
                DebuggerBreakpoint breakpoint = SetBreakpointAndSleep(
                    debuggee.Id, TestApplication.MainClass,
                    TestApplication.LoopMiddle, condition);

                // Checks that the breakpoint has a condition.
                Assert.Equal(breakpoint.Condition, condition);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(TestApplication.GetLoopUrl(app, 10));
                }

                DebuggerBreakpoint newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                // Checks that the breakpoint has been hit.
                Assert.True(newBp.IsFinalState);
                // Checks that "i" is 0 when the breakpoint is hit.
                Debugger.V2.StackFrame firstFrame = newBp.StackFrames[0];
                DebuggerVariable iVariable = firstFrame.Locals.First(local => local.Name == "i");
                Assert.Equal("0", iVariable.Value);
            }
        }

        [Fact]
        public async Task BreakpointHit_FunctionEvaluationNotPerformed()
        {
            string condition = "Hello() == \"Hello, World!\"";
            using (var app = StartTestApp(debugEnabled: true))
            {
                Debuggee debuggee = Polling.GetDebuggee(app.Module, app.Version);
                DebuggerBreakpoint breakpoint = SetBreakpointAndSleep(
                    debuggee.Id, TestApplication.MainClass,
                    TestApplication.LoopMiddle, condition);

                // Checks that the breakpoint has a condition.
                Assert.Equal(breakpoint.Condition, condition);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(TestApplication.GetLoopUrl(app, 10));
                }

                DebuggerBreakpoint newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                // Checks that the breakpoint has been hit.
                Assert.True(newBp.IsFinalState);
                // However, it should have error status set to true.
                Assert.True(newBp.Status.IsError);
                Assert.Contains("Method call for condition evaluation is disabled.", newBp.Status.Description.Format);
                Assert.Empty(newBp.StackFrames);
            }
        }

        [Fact]
        public async Task BreakpointHit_FunctionEvaluationFailure()
        {
            string condition = "NonExistentFunc() == \"Hello, World!\"";
            using (var app = StartTestApp(debugEnabled: true, methodEvaluation: true))
            {
                Debuggee debuggee = Polling.GetDebuggee(app.Module, app.Version);
                DebuggerBreakpoint breakpoint = SetBreakpointAndSleep(
                    debuggee.Id, TestApplication.MainClass,
                    TestApplication.LoopMiddle, condition);

                // Checks that the breakpoint has a condition.
                Assert.Equal(breakpoint.Condition, condition);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(TestApplication.GetLoopUrl(app, 10));
                }

                DebuggerBreakpoint newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                // Checks that the breakpoint has been hit.
                Assert.True(newBp.IsFinalState);
                // However, it should have error status set to true.
                Assert.True(newBp.Status.IsError);
                Assert.Empty(newBp.StackFrames);
            }
        }

        [Fact]
        public async Task BreakpointHit_IndexerAccessCondition()
        {
            int i = 10;
            string condition = $"testList[1] == \"List{i}1\"";
            using (var app = StartTestApp(debugEnabled: true))
            {
                Debuggee debuggee = Polling.GetDebuggee(app.Module, app.Version);
                DebuggerBreakpoint breakpoint = SetBreakpointAndSleep(
                    debuggee.Id, TestApplication.MainClass,
                    TestApplication.EchoBottomLine, condition);

                // Checks that the breakpoint has a condition.
                Assert.Equal(breakpoint.Condition, condition);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(TestApplication.GetEchoUrl(app, i));
                }

                DebuggerBreakpoint newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);

                // Check that the breakpoint has been hit.
                Assert.True(newBp.IsFinalState);
            }
        }

        [Fact]
        public async Task BreakpointHit_IndexerAccessConditionFailed()
        {
            int i = 10;
            string condition = $"testList[1] == \"List{i}2\"";
            using (var app = StartTestApp(debugEnabled: true, methodEvaluation: true))
            {
                Debuggee debuggee = Polling.GetDebuggee(app.Module, app.Version);
                DebuggerBreakpoint breakpoint = SetBreakpointAndSleep(
                    debuggee.Id, TestApplication.MainClass,
                    TestApplication.EchoBottomLine, condition);

                // Checks that the breakpoint has a condition.
                Assert.Equal(breakpoint.Condition, condition);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(TestApplication.GetEchoUrl(app, i));
                }

                Assert.Throws<TimeoutException>(() =>
                    Polling.GetBreakpoint(debuggee.Id, breakpoint.Id));
            }
        }

        [Fact]
        public async Task BreakpointsSet_ConditionMultiple()
        {
            // In this tests, 2 of the breakpoints have their conditions satisfied
            // while 1 does not.
            int i = 10;
            using (var app = StartTestApp(debugEnabled: true, methodEvaluation: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint1 = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass,
                    TestApplication.EchoBottomLine, $"testList[1] == \"List{i}1\"");
                var breakpoint2 = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass,
                    TestApplication.EchoBottomLine, $"testList[1] == \"List{i}2\"");
                var breakpoint3 = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass,
                    TestApplication.EchoBottomLine, $"testDictionary[Key{i}2] == 2");

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(TestApplication.GetEchoUrl(app, i));

                    Assert.Throws<TimeoutException>(() =>
                        Polling.GetBreakpoint(debuggee.Id, breakpoint2.Id));

                    var newBp1 = Polling.GetBreakpoint(debuggee.Id, breakpoint1.Id);
                    Assert.True(newBp1.IsFinalState);

                    var newBp3 = Polling.GetBreakpoint(debuggee.Id, breakpoint3.Id);
                    Assert.True(newBp3.IsFinalState);
                }
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

        /// <summary>
        /// Test that if the agent and debugger fail and are not running
        /// the users application will still be up and running.
        /// </summary>
        [Fact]
        public async Task DebuggerDies_AppLives()
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.HelloLine);

                using (HttpClient client = new HttpClient())
                {
                    // Hit the app and ensure the breakpoint has been hit.
                    await client.GetAsync(app.AppUrlBase);
                    var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);
                    Assert.True(newBp.IsFinalState);

                    // Kill the debugger process and agent process.
                    var debugProcess = app.GetDebuggerProcess();
                    var agentProcess = app.GetAgentProcess();

                    debugProcess.Kill();
                    if (!agentProcess.HasExited)
                    {
                        // Killing the agent will kill the debugger and visa versa.
                        // This is just a sanity check that all processes are no
                        // longer running.
                        agentProcess.Kill();
                    }

                    // Ensure both the agent and debugger have shutdown.
                    Assert.True(debugProcess.HasExited);
                    Assert.True(agentProcess.HasExited);

                    // Ensure the app is still alive.
                    var result = await client.GetAsync(app.AppUrlBase);
                    Assert.Equal("Hello, World!", result.Content.ReadAsStringAsync().Result);
                }
            }
        }

        /// <summary>
        /// Test that if the running application dies then the 
        /// agent and debugger will also shut down.
        /// </summary>
        [Fact]
        public async Task AppDies_DebuggerDies()
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.HelloLine);

                using (HttpClient client = new HttpClient())
                {
                    // Hit the app and ensure the breakpoint has been hit.
                    await client.GetAsync(app.AppUrlBase);
                    var newBp = Polling.GetBreakpoint(debuggee.Id, breakpoint.Id);
                    Assert.True(newBp.IsFinalState);

                    var debugProcess = app.GetDebuggerProcess();
                    var agentProcess = app.GetAgentProcess();

                    // Shut down the running application.
                    app.ShutdownApp();

                    // The loops allows for about 60 seconds total.  This is a generous limit,
                    // generally it takes less than 10 seconds.  However, it has been observed taking
                    // a lot longer.
                    int counter = 0;
                    while ((!debugProcess.HasExited || !agentProcess.HasExited) && counter++ < 60)
                    {
                        Thread.Sleep(TimeSpan.FromSeconds(1));
                    }

                    // Ensure both the agent and debugger have shutdown.
                    Assert.True(debugProcess.HasExited);
                    Assert.True(agentProcess.HasExited);
                }
            }
        }
    }
}
