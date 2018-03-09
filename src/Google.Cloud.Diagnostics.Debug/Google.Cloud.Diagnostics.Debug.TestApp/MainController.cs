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
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;

namespace Google.Cloud.Diagnostics.Debug.TestApp
{
    public class MainController : Controller
    {
        // Static variable for testing.
        private static readonly String _privateReadOnlyString = "can you read me? not yet!";

        // Class variable for testing.
        public String publicString = "you can read me!";

        public string Hello()
        {
            return "Hello, World!";
        }

        public string Echo(string message)
        {
            Thread.Sleep(TimeSpan.FromMilliseconds(50));
            List<string> testList = new List<string>();
            HashSet<string> testSet = new HashSet<string>();
            Dictionary<string, int> testDictionary = new Dictionary<string, int>();
            for (int i = 0; i < 5; i += 1)
            {
                testList.Add($"List{message}{i}");
                testSet.Add($"Set{message}{i}");
                testDictionary[$"Key{message}{i}"] = i;
            }
            return message;
        }

        public string Shutdown()
        {
            Program.Shutdown();
            return "Bye!";
        }

        public int ProcessId()
        {
            return Process.GetCurrentProcess().Id;
        }
    }
}
