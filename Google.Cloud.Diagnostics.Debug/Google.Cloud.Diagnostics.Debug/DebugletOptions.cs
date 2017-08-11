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

using CommandLine;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// Options for starting a <see cref="Debuglet"/>.
    /// </summary>
    internal class DebugletOptions
    {
        [Option("module", Required = true, HelpText = "The name of the application to debug.")]
        public string Module { get; set; }

        [Option("version", Required = true, HelpText = "The version of the application to debug.")]
        public string Version { get; set; }

        [Option("debugger", Required = true, HelpText = "A path to the debugger to use.")]
        public string Debugger { get; set; }

        [Option("process-id", Required = true, HelpText = "The id of the process to debug.")]
        public string ProcessId { get; set; }

        [Option("project-id",
            HelpText = "The Google Cloud Console project the debuggee is associated with")]
        public string ProjectId { get; set; }

        [Option("wait-time", Default = 2, 
            HelpText = "The amount of time to wait before checking for new breakpoints in seconds.")]
        public int WaitTime { get; set; }

        /// <summary>
        /// Parse a <see cref="DebugletOptions"/> from command line arguments.
        /// </summary>
        public static DebugletOptions Parse(string[] args)
        {
            var result = Parser.Default.ParseArguments<DebugletOptions>(args);
            var options = new DebugletOptions();
            // TODO(talarico): Add better validation, file exists, ect.
            result.WithParsed((o) => options = o);
            return options;
        }
    }
}
