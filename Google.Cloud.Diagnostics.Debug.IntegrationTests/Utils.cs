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

using System;
using System.IO;

namespace Google.Cloud.Diagnostics.Debug.IntegrationTests
{
    /// <summary>
    /// Utility classes for integration tests.
    /// </summary>
    public class Utils
    {
#if DEBUG
        private const string _mode = "Debug";
#else
        private const string _mode = "Release";
#endif

        /// <summary>
        /// Gets the Google Cloud Console project id to run the test as.
        /// </summary>
        public static string GetProjectIdFromEnvironment() => GetFromEnvironment("TEST_PROJECT");

        /// <summary>
        /// Gets the location of the application to debug.  Defaults to the Google.Cloud.Diagnostics.Debug.TestApp.
        /// This can be overridden with the environment variable 'TEST_APPLICATION'.
        /// </summary>
        public static string GetApplication()
        {
            try
            {
                return GetFromEnvironment("TEST_APPLICATION");
            }
            catch (InvalidOperationException)
            {
                // TODO(talarico): Pull out netcoreapp1.1
                return GetRootDirectory() + $"\\Google.Cloud.Diagnostics.Debug.TestApp\\bin\\{_mode}\\netcoreapp1.1\\Google.Cloud.Diagnostics.Debug.TestApp.dll";
            }
        }

        /// <summary>
        /// Gets the location of the debugger.  Defaults to the debugger in this project
        /// This can be overridden with the environment variable 'TEST_DEBUGGER'.
        /// </summary>
        public static string GetDebugger()
        {
            try
            {
                return GetFromEnvironment("TEST_DEBUGGER");
            }
            catch (InvalidOperationException)
            {
                return GetRootDirectory() + $"\\x64\\{_mode}\\google_cloud_debugger.exe";
            }
        }

        /// <summary>
        /// Gets the value of an environment variable.
        /// </summary>
        public static string GetFromEnvironment(string key)
        {
            string value = Environment.GetEnvironmentVariable(key);
            if (string.IsNullOrEmpty(value))
            {
                throw new InvalidOperationException($"Ensure environment variable {key} is set.");
            }
            return value;
        }

        /// <summary>
        /// Gets the root directory of the solution this project is in.
        /// </summary>
        private static string GetRootDirectory() =>
            Path.GetFullPath(Directory.GetCurrentDirectory() + "\\..\\..\\..\\..");
    }
}
