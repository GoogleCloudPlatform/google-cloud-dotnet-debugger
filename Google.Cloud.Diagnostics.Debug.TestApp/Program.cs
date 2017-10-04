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

using Microsoft.AspNetCore.Hosting;
using System.IO;
using System.Threading;

namespace Google.Cloud.Diagnostics.Debug.TestApp
{
    /// <summary>
    /// A program to test the debugger against.  Not that changes to this 
    /// may break the integration tests.
    /// </summary>
    public class Program
    {
        private static CancellationTokenSource _cts = new CancellationTokenSource();
        public static void Main(string[] args)
        {
            new WebHostBuilder()
               .UseKestrel()
               .UseContentRoot(Directory.GetCurrentDirectory())
               .UseIISIntegration()
               .UseStartup<Startup>()
               .Build()
               .Run(_cts.Token);
        }

        public static void Shutdown() =>_cts.Cancel();
    }
}