// Copyright 2018 Google Inc. All Rights Reserved.
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

using Google.Cloud.Diagnostics.Debug.IntegrationTests;
using System;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.LongRunningTests
{
    public class DebuggerTests : DebuggerTestBase
    {
        /// <summary>
        /// The amount of time for a hanging get to the debugger API to timeout
        /// which will trigger another reload of active breakpoints.
        /// </summary>
        private static readonly TimeSpan _hangingGetTimeout = TimeSpan.FromMinutes(1);

        [Fact]
        public async Task BreakpointHit_Wait()
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint = SetBreakpointAndSleep(debuggee.Id, TestApplication.MainClass, TestApplication.HelloLine);

                // Sleep for long period to ensure multiple get calls to the Debugger API. 
                Thread.Sleep(_hangingGetTimeout);

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
        public async Task MultipleBreakpointsHit_Wait()
        {
            using (var app = StartTestApp(debugEnabled: true))
            {
                var debuggee = Polling.GetDebuggee(app.Module, app.Version);
                var breakpoint1 = SetBreakpoint(debuggee.Id, TestApplication.MainClass, TestApplication.HelloLine);
                var breakpoint2 = SetBreakpoint(debuggee.Id, TestApplication.MainClass, TestApplication.EchoTopLine);

                // Let the first two breakpoints get picked up by the server before
                // setting the last one.
                Thread.Sleep(TimeSpan.FromSeconds(5));
                var breakpoint3 = SetBreakpoint(debuggee.Id, TestApplication.MainClass, TestApplication.PidLine);

                // Sleep for long period to ensure multiple get calls to the Debugger API. 
                Thread.Sleep(_hangingGetTimeout);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync(app.AppUrlBase);
                }

                var newBp1 = Polling.GetBreakpoint(debuggee.Id, breakpoint1.Id);
                Assert.True(newBp1.IsFinalState);

                // Ensure these breakpoints aren't final.
                var newBp2 = Polling.GetBreakpoint(debuggee.Id, breakpoint2.Id, isFinal: false);
                var newBp3 = Polling.GetBreakpoint(debuggee.Id, breakpoint3.Id, isFinal: false);
                Assert.False(newBp2.IsFinalState);
                Assert.False(newBp3.IsFinalState);

                // Sleep for long period to ensure multiple get calls to the Debugger API. 
                Thread.Sleep(_hangingGetTimeout);

                using (HttpClient client = new HttpClient())
                {
                    await client.GetAsync($"{app.AppUrlEcho}/1");
                    await client.GetAsync(app.AppUrlProcessId);
                }

                newBp2 = Polling.GetBreakpoint(debuggee.Id, breakpoint2.Id);
                newBp3 = Polling.GetBreakpoint(debuggee.Id, breakpoint3.Id);
                Assert.True(newBp2.IsFinalState);
                Assert.True(newBp3.IsFinalState);
            }
        }
    }
}
