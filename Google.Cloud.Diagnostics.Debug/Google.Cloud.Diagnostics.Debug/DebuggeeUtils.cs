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

using Google.Api.Gax;
using Google.Cloud.Debugger.V2;
using Google.Cloud.DevTools.Source.V1;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// Utility functions for <see cref="Debuggee"/>s.
    /// </summary>
    internal class DebuggeeUtils
    {
        /// <summary>
        /// Creates a <see cref="Debuggee"/>.
        /// </summary>
        /// <param name="projectId">The Google Cloud Console project.</param>
        /// <param name="module">The name of the application.</param>
        /// <param name="version">The version of the application.</param>
        /// <param name="sourceContext">The source context for the users application.</param>
        public static Debuggee CreateDebuggee(
            string projectId, string module, string version, SourceContext sourceContext)
        {
            var debuggee = new Debuggee
            {
                AgentVersion = GetAgentVersion(Common.Platform),
                Description = GetDescription(module, version),
                Project = projectId,
                Labels = { { GetLabels(projectId, module, version) } },
                SourceContexts = { { sourceContext ?? new SourceContext() } },
            };
            
            debuggee.Uniquifier = GetUniquifier(debuggee);
            return debuggee;
        }

        /// <summary>
        /// Gets the uniquifier to uniquely identify this application. 
        /// </summary>
        public static string GetUniquifier(Debuggee debuggee)
        {
            using (SHA1 sha = SHA1.Create())
            {
                List<byte> bytesList = new List<byte>();
                bytesList.AddRange(Encoding.UTF8.GetBytes(debuggee.AgentVersion));
                bytesList.AddRange(Encoding.UTF8.GetBytes(debuggee.Description));
                bytesList.AddRange(Encoding.UTF8.GetBytes(debuggee.Project));
                var orderedLabels = debuggee.Labels.Select(x => $"{x.Key}:{x.Value}").OrderBy(x => x);
                bytesList.AddRange(Encoding.UTF8.GetBytes(string.Join(",", orderedLabels)));
                bytesList.AddRange(Encoding.UTF8.GetBytes(debuggee.SourceContexts.ToString()));

                byte[] bytes = sha.ComputeHash(bytesList.ToArray());
                return Encoding.UTF8.GetString(bytes);
            }
        }

        /// <summary>
        /// Gets a human readable description of the module and version.
        /// </summary>
        public static string GetDescription(string module, string version) {
            GaxPreconditions.CheckNotNullOrEmpty(module, nameof(module));
            return string.IsNullOrWhiteSpace(version) ? module : $"{module} - {version}";
        }

        /// <summary>
        /// Gets labels to describe the debuggee.
        /// </summary>
        public static IDictionary<string, string> GetLabels(string projectId, string module, string version)
        {
            GaxPreconditions.CheckNotNullOrEmpty(projectId, nameof(projectId));
            GaxPreconditions.CheckNotNullOrEmpty(module, nameof(module));

            var labels =  new Dictionary<string, string>
            {
                { "projectid", projectId },
                { "module", module },
            };

            if (!string.IsNullOrWhiteSpace(version))
            {
                labels["version"] = version;
            }
            return labels;
        }

        /// <summary>
        /// Gets the version of this application. 
        /// </summary>
        public static string GetVersion() =>
            typeof(DebuggeeUtils).GetTypeInfo().Assembly
                .GetCustomAttribute<AssemblyInformationalVersionAttribute>()
                .InformationalVersion;

        /// <summary>
        /// Gets the full agent version of this application.
        /// </summary>
        public static string GetAgentVersion(Platform platform)
        {
            GaxPreconditions.CheckNotNull(platform, nameof(platform));
            var version = GetVersion();
            var platformType = GetPlatform(platform);
            var platformString = platformType != null ? $"-{platformType}" : "";
            return $"google.com/csharp{platformString}/v{version}";
        }

        /// <summary>
        /// Gets a human readable type of a <see cref="Platform"/>
        /// </summary>
        public static string GetPlatform(Platform platform)
        {
            GaxPreconditions.CheckNotNull(platform, nameof(platform));
            switch (platform.Type)
            {
                case PlatformType.Gae:
                    return "gae";
                case PlatformType.Gce:
                    return "gce";
                case PlatformType.Gke:
                    return "gke";
                default:
                    return null;
            }
        }
    }
}
