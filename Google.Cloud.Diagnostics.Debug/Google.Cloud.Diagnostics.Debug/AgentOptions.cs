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
using Google.Api.Gax;
using System;
using System.IO;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// Options for starting a <see cref="Debuglet"/>.
    /// </summary>
    public class AgentOptions
    {
        /// <summary>The prefix for environment variables.</summary>
        public static readonly string EnvironmentVariablePrefix = "STACKDRIVER_DEBUGGER";

        /// <summary>An environment variable that can set the module.</summary>
        public static readonly string ModuleEnvironmentVariable = $"{EnvironmentVariablePrefix}_MODULE";

        /// <summary>An environment variable that can set the version.</summary>
        public static readonly string VersionEnvironmentVariable = $"{EnvironmentVariablePrefix}_VERSION";

        /// <summary>An environment variable that can set the project ID.</summary>
        public static readonly string ProjectEnvironmentVariable = $"{EnvironmentVariablePrefix}_PROJECT";

        /// <summary>An environment variable that can set the debugger path.</summary>
        public static readonly string DebuggerEnvironmentVariable = $"{EnvironmentVariablePrefix}_DEBUGGER";

        // If given this option, the debugger will not perform property evaluation.
        public const string PropertyEvaluationOption = "--property-evaluation";

        // If given this option, the debugger will start and debug an application using this path.
        public const string ApplicationPathOption = "--application-path";

        // If given this option, the debugger will attach to a running application using this process ID.
        public const string ApplicationIdOption = "--application-id";

        [Option("module", Required = true, HelpText = "The name of the application to debug.")]
        public string Module { get; set; }

        [Option("version", Required = true, HelpText = "The version of the application to debug.")]
        public string Version { get; set; }

        [Option("debugger", Required = true, HelpText = "A path to the debugger to use.")]
        public string Debugger { get; set; }

        [Option("application-path",
            HelpText = "A path to the .NET CORE application dll to be debugged." +
            " This will delay the start up of the application as the debugger needs" +
            " to be set up to ensure that startup code will be debugged.")]
        public string ApplicationPath { get; set; }

        [Option("application-id",
            HelpText = "Process ID of a running .NET CORE application to be debugged.")]
        public int? ApplicationId { get; set; }

        [Option("project-id",
            HelpText = "The Google Cloud Console project the debuggee is associated with.")]
        public string ProjectId { get; set; }

        [Option("property-evaluation",
            HelpText = "If set, the debugger will evaluate object's properties.")]
        public bool PropertyEvaluation { get; set; }

        [Option("wait-time", Default = 2, 
            HelpText = "The amount of time to wait before checking for new breakpoints in seconds.")]
        public int WaitTime { get; set; }

        /// <summary>
        /// Returns the processed arguments to pass to the debugger.
        /// It will either be "application-path path (-property-evaluation)"
        /// or "application-id id (-property-evaluation).
        /// </summary>
        public string DebuggerArguments
        {
            get
            {
                if (ApplicationId.HasValue)
                {
                    return PropertyEvaluation ? $"{ApplicationIdOption}={ApplicationId} {PropertyEvaluationOption}"
                        : $"{ApplicationIdOption}={ApplicationId}";
                }
                else
                {
                    return PropertyEvaluation ? $"{ApplicationPathOption}={ApplicationPath} {PropertyEvaluationOption}"
                        : $"{ApplicationPathOption}={ApplicationPath}";
                }
            }
        }

        /// <summary>
        /// Parse a <see cref="AgentOptions"/> from command line arguments.
        /// </summary>
        public static AgentOptions Parse(string[] args)
        {
            var result = Parser.Default.ParseArguments<AgentOptions>(args);
            var options = new AgentOptions();
            result.WithParsed((o) => 
            {
                o.Module = GaxPreconditions.CheckNotNullOrEmpty(o.Module ?? GetModule(), nameof(o.Module));
                o.Version = GaxPreconditions.CheckNotNullOrEmpty(o.Version ?? GetVersion(), nameof(o.Version));
                o.ProjectId = GaxPreconditions.CheckNotNullOrEmpty(o.ProjectId ?? GetProject(), nameof(o.ProjectId));
                o.Debugger = GaxPreconditions.CheckNotNullOrEmpty(o.Debugger ?? GetDebugger(), nameof(o.Debugger));
                GaxPreconditions.CheckArgumentRange(o.WaitTime, nameof(o.WaitTime), 0, int.MaxValue);

                if (!File.Exists(o.Debugger))
                {
                    throw new FileNotFoundException($"Debugger file not found: '{o.Debugger}'");
                }

                if ((string.IsNullOrWhiteSpace(o.ApplicationPath) && !o.ApplicationId.HasValue)
                    || (!string.IsNullOrWhiteSpace(o.ApplicationPath) && o.ApplicationId.HasValue))
                {
                    throw new ArgumentException("Please supply either the path to the .NET CORE application dll"
                        + " or the process ID of a running application, NOT both.");
                }

                if (!string.IsNullOrWhiteSpace(o.ApplicationPath))
                {
                    o.ApplicationPath = Path.GetFullPath(o.ApplicationPath);
                    if (!File.Exists(o.ApplicationPath))
                    {
                        throw new FileNotFoundException($"Application file not found: '{o.ApplicationPath}'");
                    }
                }

                options = o;
            });
            return options;
        }

        /// <summary>
        /// Attempts to get the module from the environment. 
        /// Will first try to get the module from an environment variable, if the
        /// environment variable does not exist it will attempt to get the module from
        /// the platform.
        /// </summary>
        /// <param name="platform">The platform to use, if not set a default will be used.</param>
        /// <returns>The module or null if none could be found.</returns>
        internal static string GetModule(Platform platform = null)
        {
            platform = platform ?? Common.Platform;
            var module = Environment.GetEnvironmentVariable(ModuleEnvironmentVariable);
            if (module == null && platform.Type == PlatformType.Gae)
            {
                return platform.GaeDetails.ServiceId;
            }
            return module;
        }

        /// <summary>
        /// Attempts to get the version from the environment. 
        /// Will first try to get the version from an environment variable, if the
        /// environment variable does not exist it will attempt to get the version from
        /// the platform.
        /// </summary>
        /// <param name="platform">The platform to use, if not set a default will be used.</param>
        /// <returns>The version or null if none could be found.</returns>
        internal static string GetVersion(Platform platform = null)
        {
            platform = platform ?? Common.Platform;
            var module = Environment.GetEnvironmentVariable(VersionEnvironmentVariable);
            if (module == null && platform.Type == PlatformType.Gae)
            {
                return platform.GaeDetails.VersionId;
            }
            return module;
        }

        /// <summary>
        /// Attempts to get the project ID from the environment. 
        /// Will first try to get the project ID from an environment variable, if the
        /// environment variable does not exist it will attempt to get the project ID from
        /// the platform.
        /// </summary>
        /// <param name="platform">The platform to use, if not set a default will be used.</param>
        /// <returns>The project ID or null if none could be found.</returns>
        internal static string GetProject(Platform platform = null)
        {
            platform = platform ?? Common.Platform;
            return Environment.GetEnvironmentVariable(ProjectEnvironmentVariable) ?? platform.ProjectId;
        }

        /// <summary>
        /// Attempts to get the debugger from an environment variable. 
        /// </summary>
        /// <returns>The debugger or null if none could be found.</returns>
        internal static string GetDebugger() =>
            Environment.GetEnvironmentVariable(DebuggerEnvironmentVariable);
    }
}
