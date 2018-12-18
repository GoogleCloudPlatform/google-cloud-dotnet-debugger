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
using System.Collections.Generic;
using Google.Cloud.Logging.V2;
using StackdriverBreakpoint = Google.Cloud.Debugger.V2.Breakpoint;
using Google.Cloud.Logging.Type;
using Google.Api.Gax;
using Google.Api;
using System.Linq;

namespace Google.Cloud.Diagnostics.Debug
{
    class LoggingClient : ILoggingClient
    {
        private static readonly Dictionary<StackdriverBreakpoint.Types.LogLevel, LogSeverity> _logSeverityConversion
            = new Dictionary<StackdriverBreakpoint.Types.LogLevel, LogSeverity>()
        { { StackdriverBreakpoint.Types.LogLevel.Info, LogSeverity.Info },
          {StackdriverBreakpoint.Types.LogLevel.Warning, LogSeverity.Warning },
          {StackdriverBreakpoint.Types.LogLevel.Error, LogSeverity.Error } };

        private readonly LoggingServiceV2Client _logClient;
        private readonly AgentOptions _options;
        private readonly LogName _logName;

        internal LoggingClient(AgentOptions options, LoggingServiceV2Client loggingClient = null)
        {
            _logClient = loggingClient ?? LoggingServiceV2Client.Create();
            _options = GaxPreconditions.CheckNotNull(options, nameof(options));
            _logName = new LogName(_options.ProjectId, _options.LogName ??
                $"{_options.Module}-{_options.Version}-debug-log");
        }
        
        /// <summary>
        /// Substitutes the log message format in breakpoint and writes
        /// the result as a log entry to the log _logName.
        /// </summary>
        /// <returns>WriteLogEntriesResponse from the API.</returns>
        public WriteLogEntriesResponse WriteLogEntry(StackdriverBreakpoint breakpoint)
        {
            LogEntry logEntry = new LogEntry
            {
                LogName = _logName.ToString(),
                Severity = _logSeverityConversion[breakpoint.LogLevel],
                TextPayload = SubstituteLogMessageFormat(
                    breakpoint.LogMessageFormat,
                    breakpoint.EvaluatedExpressions.ToList())
            };
            // TODO(quoct): Detect whether we are on gke and use gke_container.
            MonitoredResource resource = new MonitoredResource { Type = "global" };
            return _logClient.WriteLogEntries(LogNameOneof.From(_logName), resource, null, new[] { logEntry });
        }

        /// <summary>
        /// Substitutes the $0, $1, etc. in messageFormat with expressions
        /// from evaluatedExpressions.
        /// </summary>
        /// <returns>Formatted log message with expressions substituted.</returns>
        private string SubstituteLogMessageFormat(string messageFormat, List<Debugger.V2.Variable> evaluatedExpressions)
        {
            string result = "LOGPOINT: ";
            int offset = 0;
            int i = 0;
            while (i < messageFormat.Length)
            {
                if (messageFormat[i] != '$')
                {
                    i += 1;
                    continue;
                }

                // We encounter a $.
                result += messageFormat.Substring(offset, i - offset);

                char nextChar = messageFormat[i + 1];
                if (nextChar == '$')
                {
                    // Escape the current $.
                    result += '$';
                    i += 2;
                    offset = i;
                    continue;
                }

                if (!Char.IsDigit(nextChar))
                {
                    offset = i;
                    i += 1;
                    continue;
                }

                // We have a number followed by a $. Time to substitute!
                string currentNumberString = "";
                i += 1;
                while (i < messageFormat.Length && Char.IsDigit(messageFormat[i]))
                {
                    currentNumberString += messageFormat[i];
                    i += 1;
                }
                offset = i;

                int currentNumber = Int32.Parse(currentNumberString);
                if (currentNumber < evaluatedExpressions.Count)
                {
                    result += FormatVariable(evaluatedExpressions[currentNumber]);
                }
                else
                {
                    result += $"{{{currentNumber} cannot be evaluated}}";
                }
            }

            if (offset < messageFormat.Length)
            {
                result += messageFormat.Substring(offset);
            }

            return result;
        }

        /// <summary>
        /// Formats the variable into a more readable string,
        /// especially if the variable only has members and no value.
        /// </summary>
        /// <returns>Formatted string representing the variable.</returns>
        private string FormatVariable(Debugger.V2.Variable variable)
        {
            if (variable.Status?.IsError ?? false)
            {
                return $"\"Error evaluating {variable.Name}: {variable.Status?.Description?.Format}\"";
            }

            if (!string.IsNullOrWhiteSpace(variable.Value))
            {
                return variable.Value;
            }

            string result = "[ ";
            foreach (Debugger.V2.Variable member in variable.Members)
            {
                result += $"{member.Name} ({member.Type}): {FormatVariable(member)}, ";
            }
            result = result.TrimEnd(new char[] { ',', ' ' });
            result += "]";

            return result;
        }
    }
}
