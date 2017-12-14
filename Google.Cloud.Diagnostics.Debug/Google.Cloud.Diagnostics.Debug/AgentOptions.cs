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
using CommandLine.Text;
using Google.Api.Gax;
using Google.Cloud.DevTools.Source.V1;
using System;
using System.IO;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// Options for starting a <see cref="Debuglet"/>.
    /// </summary>
    public class AgentOptions
    {
        /// <summary>The name of the source context file.</summary>
        public static readonly string SourceContextFileName = "source-context.json";

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

        /// <summary>An environment variable that can set the source context file location.</summary>
        public static readonly string SourceContextEnvironmentVariable = $"{EnvironmentVariablePrefix}_SOURCE_CONTEXT";

        [Option("module", Required = true, HelpText = "The name of the application to debug.")]
        public string Module { get; set; }

        [Option("version", Required = true, HelpText = "The version of the application to debug.")]
        public string Version { get; set; }

        [Option("debugger", Required = true, HelpText = "A path to the debugger to use.")]
        public string Debugger { get; set; }

        [Option("application-start-command",
            HelpText = "A command to start a .NET CORE application to be debugged." +
            " This will delay the start up of the application as the debugger needs" +
            " to be set up to ensure that startup code will be debugged.")]
        public string ApplicationStartCommand { get; set; }

        [Option("application-id",
            HelpText = "Process ID of a running .NET CORE application to be debugged.")]
        public int? ApplicationId { get; set; }

        [Option("project-id",
            HelpText = "The Google Cloud Console project the debuggee is associated with.")]
        public string ProjectId { get; set; }

        [Option("property-evaluation",
            HelpText = "If set, the debugger will evaluate object's properties.")]
        public bool PropertyEvaluation { get; set; }

        [Option("source-context",
            HelpText = "The location of the source context file. See: " +
            "https://cloud.google.com/debugger/docs/source-context")]
        public string SourceContextFile { get; set; }

        [HelpOption]
        public string Usage() => HelpText.AutoBuild(
            this, (HelpText helpText) => HelpText.DefaultParsingErrorsHandler(this, helpText));

        /// <summary>The parsed source context from the <see cref="SourceContextFile"/>.</summary>
        public SourceContext SourceContext { get; set; }

        /// <summary>
        /// Parse a <see cref="AgentOptions"/> from command line arguments.
        /// </summary>
        public static AgentOptions Parse(string[] args)
        {
            var options = new AgentOptions();
            if (Parser.Default.ParseArgumentsStrict(args, options))
            {
                options.Module = GaxPreconditions.CheckNotNullOrEmpty(options.Module ?? GetModule(), nameof(options.Module));
                options.Version = GaxPreconditions.CheckNotNullOrEmpty(options.Version ?? GetVersion(), nameof(options.Version));
                options.ProjectId = GaxPreconditions.CheckNotNullOrEmpty(options.ProjectId ?? GetProject(), nameof(options.ProjectId));
                options.Debugger = GaxPreconditions.CheckNotNullOrEmpty(options.Debugger ?? GetDebugger(), nameof(options.Debugger));

                if (!File.Exists(options.Debugger))
                {
                    throw new FileNotFoundException($"Debugger file not found: '{options.Debugger}'");
                }

                if ((string.IsNullOrWhiteSpace(options.ApplicationStartCommand) && !options.ApplicationId.HasValue)
                    || (!string.IsNullOrWhiteSpace(options.ApplicationStartCommand) && options.ApplicationId.HasValue))
                {
                    throw new ArgumentException("Please supply either a command to start a .NET CORE application"
                        + " or the process ID of a running application, NOT both.");
                }

                options.SourceContextFile = GetSourceContextFile(options.SourceContextFile);
                if (File.Exists(options.SourceContextFile))
                {
                    var contents = File.ReadAllText(options.SourceContextFile);
                    options.SourceContext = SourceContext.Parser.ParseJson(contents);
                }
            }
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

        /// <summary>
        /// Attempts to get the location of a source context file.
        /// It will first look at the passed in file (from the parameters). 
        /// If not found it will look at the <see cref="SourceContextEnvironmentVariable"/>.
        /// Lastly it will try and look in this agents application directory.
        /// </summary>
        /// <returns>The location of the source context file or null if non can be found.</returns>
        internal static string GetSourceContextFile(string file)
        {
            // look at passed in var
            // look at env variable
            // look in root of this project

            if (File.Exists(file))
            {
                return file;
            }

            file = Environment.GetEnvironmentVariable(SourceContextEnvironmentVariable);
            if (File.Exists(file))
            {
                return file;
            }

            file = Path.Combine(AppContext.BaseDirectory, SourceContextFileName);
            if (File.Exists(file))
            {
                return file;
            }

            return null;
        }
    }
}
