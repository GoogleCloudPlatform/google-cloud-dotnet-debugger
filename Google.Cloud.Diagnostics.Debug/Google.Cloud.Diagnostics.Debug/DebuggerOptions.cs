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

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// Options for starting a debugger.
    /// </summary>
    public class DebuggerOptions
    {
        // If given this option, the debugger will not perform property evaluation.
        public const string PropertyEvaluationOption = "--property-evaluation";

        // If given this option, the debugger will use this command to start the application to debug.
        public const string ApplicationStartCommandOption = "--application-start-command";

        // If given this option, the debugger will attach to a running application using this process ID.
        public const string ApplicationIdOption = "--application-id";

        // The name of the pipe the debugger will attach to.
        public const string PipeNameOption = "--pipe-name";

        public bool PropertyEvaluation { get; private set; }

        public string ApplicationStartCommand { get; private set; }

        public int? ApplicationId { get; private set; }

        public string PipeName { get; private set; }

        public static DebuggerOptions FromAgentOptions(AgentOptions options)
        {
            if ((string.IsNullOrWhiteSpace(options.ApplicationStartCommand) && !options.ApplicationId.HasValue)
                    || (!string.IsNullOrWhiteSpace(options.ApplicationStartCommand) && options.ApplicationId.HasValue))
            {
                throw new ArgumentException("TODO.");
            }

            return new DebuggerOptions
            {
                PropertyEvaluation = options.PropertyEvaluation,
                ApplicationStartCommand = options.ApplicationStartCommand,
                ApplicationId = options.ApplicationId,
                PipeName = CreatePipeName()
            };
        }

        private static string CreatePipeName() => $"{Constants.PipeName}-{Guid.NewGuid()}";

        /// <summary>
        /// TODO
        /// Returns the processed arguments to pass to the debugger.
        /// It will either be "application-start-command path (-property-evaluation)"
        /// or "application-id id (-property-evaluation).
        /// </summary>
        public override string ToString()
        {
            string options = $"{PipeNameOption}={PipeName}";
            if (ApplicationId.HasValue)
            {
                options += $"{ApplicationIdOption}={ApplicationId} ";
            }
            else
            {
                options += $"{ApplicationStartCommandOption}=\"{ApplicationStartCommand}\" ";
            }

            if (PropertyEvaluation)
            {
                options += $"{PropertyEvaluationOption} ";
            }
            return options;
        }

    }
}
