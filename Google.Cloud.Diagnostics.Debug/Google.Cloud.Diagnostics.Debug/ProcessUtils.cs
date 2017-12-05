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

using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// This class defines helper methods for starting sub-processes and getting the output from the processes.
    /// </summary>
    public static class ProcessUtils
    {
        private static ProcessStartInfo GetBaseStartInfo(string file, string args, IDictionary<string, string> environment)
        {
            // Always start the tool in the user's home directory, avoid random directories
            // coming from Visual Studio.
            ProcessStartInfo result = new ProcessStartInfo
            {
                FileName = file,
                Arguments = args,
                WorkingDirectory = Environment.GetEnvironmentVariable("USERPROFILE"),
            };

            /* TODO(quoct): Find equivalence for setting environment variables on CORECLR.
            // Customize the environment for the incoming process.
            if (environment != null)
            {
                foreach (var entry in environment)
                {
                    result.EnvironmentVariables[entry.Key] = entry.Value;
                }
            }
            */

            return result;
        }

        public static ProcessStartInfo GetStartInfoForInteractiveProcess(string file, string args, IDictionary<string, string> environment)
        {
            ProcessStartInfo startInfo = GetBaseStartInfo(file, args, environment);
            startInfo.UseShellExecute = false;
            startInfo.CreateNoWindow = true;
            return startInfo;
        }
    }
}