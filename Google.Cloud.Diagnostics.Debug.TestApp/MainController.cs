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

using Microsoft.AspNetCore.Mvc;
using System;
using System.Threading;

namespace Google.Cloud.Diagnostics.Debug.TestApp
{
    public class MainController : Controller
    {
        public string Hello()
        {
            return "Hello, World!";
        }

        public string Echo(string message)
        {
            Thread.Sleep(TimeSpan.FromMilliseconds(50));
            return message;
        }

        public string Shutdown()
        {
            Program.Shutdown();
            return "Bye!";
        }
    }
}
