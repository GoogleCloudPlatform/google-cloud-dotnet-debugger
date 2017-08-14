// Copyright 2015-2016 Google Inc. All Rights Reserved.
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


using Google.Api.Gax;
using Google.Cloud.Debugger.V2;
using Grpc.Core;
using System;
using System.Collections.Generic;

using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;

namespace Google.Cloud.Diagnostics.Debug
{
    internal class DebuggerClient
    {
        private readonly object _mutex = new object();

        private readonly DebugletOptions _options;
        private readonly Controller2Client _controlClient;

        private Debuggee _debuggee;

        private DebuggerClient(DebugletOptions options, Controller2Client controlClient = null)
        {
            _controlClient = controlClient ?? Controller2Client.Create();
            _options = GaxPreconditions.CheckNotNull(options, nameof(options));
        }

        public void Register()
        {
            lock (_mutex)
            {
                var debuggee = DebuggeeUtils.CreateDebuggee(_options.ProjectId, _options.Module, _options.Version);
                _debuggee = _controlClient.RegisterDebuggee(debuggee).Debuggee;
            }
        }

        public IEnumerable<StackdriverBreakpoint> ListBreakpoints() => 
            TryAction(() => _controlClient.ListActiveBreakpoints(_debuggee.Id).Breakpoints);

        public void UpdateBreakpoint(StackdriverBreakpoint breakpoint) =>
            TryAction(() => _controlClient.UpdateActiveBreakpoint(_debuggee.Id, breakpoint));

        private T TryAction<T>(Func<T> func)
        {
            try
            {
                return func();
            }
            catch (RpcException e) when (e.Status.StatusCode == StatusCode.NotFound)
            {
                // The debuggee was not found try to register again
                Register();
                return func();
            }
        }
    }
}
