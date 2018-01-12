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

using Google.Cloud.Debugger.V2;
using System;
using System.Threading;

namespace Google.Cloud.Diagnostics.Debug.IntegrationTests
{
    /// <summary>
    /// Class to poll for Debug objects.
    /// </summary>
    public class DebuggerPolling
    {
        /// <summary>The amount of time to before a timeout will occur when polling attempts.</summary>
        private static readonly TimeSpan _sleep = TimeSpan.FromSeconds(2);

        /// <summary>The amount of time to sleep between poll attempts.</summary>
        private static readonly TimeSpan _timeout = TimeSpan.FromSeconds(10);

        /// <summary>The Google Cloud Console project id.</summary>
        private readonly string _projectId;

        /// <summary>The Debugger Client.</summary>
        public Debugger2Client Client { get; }

        public DebuggerPolling()
        {
            Client = Debugger2Client.Create();
            _projectId = Utils.GetProjectIdFromEnvironment();
        }

        /// <summary>
        /// Gets a <see cref="Debuggee"/> with a given module and version.
        /// </summary>
        public Debuggee GetDebuggee(string module, string version)
        {
            Predicate<Debuggee> predicate = (debuggee) 
                => debuggee?.Description == DebuggeeUtils.GetDescription(module, version);

            Func<Debuggee> func = () => {
                ListDebuggeesRequest request = new ListDebuggeesRequest
                {
                    Project = _projectId
                };
                var debuggees = Client.GrpcClient.ListDebuggees(request).Debuggees;
                foreach (var debuggee in debuggees)
                {
                    if (predicate(debuggee))
                    {
                        return debuggee;
                    }
                }
                return null;
            };
            return Poll(func, predicate);
        }

        /// <summary>
        /// Gets a <see cref="Debugger.V2.Breakpoint"/>.
        /// </summary>
        /// <param name="debuggeeId">The debuggee id for the breakpoint.</param>
        /// <param name="breakpointId">The id of the breakpoint.</param>
        /// <param name="isFinal">Optional, defaults to true.  If true the method will only return the
        ///     breakpoint when it's final.</param>
        public Debugger.V2.Breakpoint GetBreakpoint(string debuggeeId, string breakpointId, bool isFinal = true)
        {
            Predicate<Debugger.V2.Breakpoint> predicate = (breakpoint)
                => isFinal ? breakpoint?.IsFinalState ?? false : breakpoint != null;

            Func<Debugger.V2.Breakpoint> func = () => {
                GetBreakpointRequest request = new GetBreakpointRequest
                {
                    DebuggeeId = debuggeeId,
                    BreakpointId = breakpointId,
                };

                return Client.GrpcClient.GetBreakpoint(request).Breakpoint;
            };
            return Poll(func, predicate);
        }

        /// <summary>
        /// Polls for a given element of type <see cref="{T}"/> that meets the 
        /// given predicate. If the element is not found before the timeout a
        /// <see cref="TimeoutException"/> will be thrown.
        /// </summary>
        private T Poll<T>(Func<T> func, Predicate<T> predicate)
        {
            var time = TimeSpan.Zero;
            while (time < _timeout)
            {
                T obj = func();
                if (predicate(obj))
                {
                    return obj;
                }
                Thread.Sleep(_sleep);
                time += _sleep;
            }
            throw new TimeoutException();
        }
    }
}
