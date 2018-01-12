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

using System;
using System.IO;

namespace Google.Cloud.Diagnostics.Debug.Tools.GenerateFiles
{
    /// <summary>
    /// Creates C# files.  This is used to help test different portable PDB 
    /// configurations and sizes.
    /// Usage:
    ///     dontnet run -- [path-to-directory] [file-name-prefix] [num-files] [num-lines-in-file]
    /// </summary>
    public class Program
    {
        /// <summary>Template to create a basic C# class.</summary>
        private const string _classTemplate = @"
namespace Google.Cloud.Diagnostics.Debug.Tools
{{
    public class {0}
    {{
        {1}
    }}
}}
";
        /// <summary>Template to create a basic C# method.</summary>
        private const string _methodTemplate = @"
        public int {0}()
        {{
            int someInt = 42;
            return someInt + {1};
        }}
";

        public static void Main(string[] args)
        {
            if (args.Length != 4)
            {
                throw new ArgumentException("Arguments should be: location, file name prefix," +
                    " number of files, number of lines per file.");
            }

            string location = args[0];
            string fileName = args[1];
            int numFiles = Int32.Parse(args[2]);
            int numLines = Int32.Parse(args[3]);

            var methods = "";
            for (int i = 0; i < numLines; i += 5)
            {
                methods += CreateMethod($"SomeMethod{i}", i);
            }

            for (int i = 0; i < numFiles; i++)
            {
                var fullPath = Path.Combine(location, $"{fileName}{i}.cs");
                var file = CreateClass($"{fileName}{i}", methods);
                File.WriteAllText(fullPath, file);
            }
        }

        private static string CreateClass(string className, string body)
            => string.Format(_classTemplate, className, body);

        private static string CreateMethod(string methodName, int rando) 
            => string.Format(_methodTemplate, methodName, rando);
    }
}
