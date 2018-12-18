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

using Google.Cloud.Logging.V2;
using StackdriverVariable = Google.Cloud.Debugger.V2.Variable;
using Moq;
using Xunit;
using Google.Api;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class LoggingClientTests
    {
        private const string _debugId = "did";
        private const string _projectId = "pid";
        private const string _module = "module";
        private const string _version = "version";
        private const string _logName = "log-name";

        private readonly AgentOptions _options;
        private readonly Mock<LoggingServiceV2Client> _mockLoggingClient;
        private readonly LoggingClient _client;
        private readonly LogName _logNameObj;
        private readonly MonitoredResource _resource;

        public LoggingClientTests()
        {
            _options = new AgentOptions
            {
                ProjectId = _projectId,
                Module = _module,
                Version = _version,
                LogName = _logName
            };
            _mockLoggingClient = new Mock<LoggingServiceV2Client>();
            _client = new LoggingClient(_options, _mockLoggingClient.Object);
            _logNameObj = new LogName(_projectId, _logName);
            _resource = new MonitoredResource { Type = "global" };
        }

        [Fact]
        public void WriteLogEntry_SimpleMessage()
        {
            string logMessageFormat = "This is a log";
            Debugger.V2.Breakpoint breakpoint = new Debugger.V2.Breakpoint()
            {
                LogLevel = Debugger.V2.Breakpoint.Types.LogLevel.Error,
                LogMessageFormat = logMessageFormat
            };
            LogEntry logEntry = new LogEntry
            {
                LogName = _logNameObj.ToString(),
                Severity = Logging.Type.LogSeverity.Error,
                TextPayload = $"LOGPOINT: {logMessageFormat}"
            };

            _client.WriteLogEntry(breakpoint);
            _mockLoggingClient.Verify(client =>
                client.WriteLogEntries(LogNameOneof.From(_logNameObj), _resource, null, new[] { logEntry }, null),
                Times.Once());
        }

        [Fact]
        public void WriteLogEntry_Error()
        {
            string logMessageFormat = "This is a log $0";
            StackdriverVariable[] evaluatedExpressions = new StackdriverVariable[]
            {
                new StackdriverVariable()
                {
                    Name = "ErrorVariable",
                    Status = new Debugger.V2.StatusMessage()
                    {
                        Description = new Debugger.V2.FormatMessage
                        {
                            Format = "This is an error"
                        },
                        IsError = true
                    }
                }
            };

            Debugger.V2.Breakpoint breakpoint = new Debugger.V2.Breakpoint()
            {
                LogLevel = Debugger.V2.Breakpoint.Types.LogLevel.Warning,
                LogMessageFormat = logMessageFormat,
                EvaluatedExpressions = { evaluatedExpressions },
            };

            LogEntry logEntry = new LogEntry
            {
                LogName = _logNameObj.ToString(),
                Severity = Logging.Type.LogSeverity.Warning,
                TextPayload = "LOGPOINT: This is a log \"Error evaluating ErrorVariable: This is an error\""
            };

            _client.WriteLogEntry(breakpoint);
            _mockLoggingClient.Verify(client =>
                client.WriteLogEntries(LogNameOneof.From(_logNameObj), _resource, null, new[] { logEntry }, null),
                Times.Once());
        }

        [Fact]
        public void WriteLogEntry_MessageWithSubstitution()
        {
            string logMessageFormat = "This is a log $0";
            StackdriverVariable[] evaluatedExpressions = new StackdriverVariable[]
            {
                new StackdriverVariable()
                {
                    Value = "test1"
                }
            };

            Debugger.V2.Breakpoint breakpoint = new Debugger.V2.Breakpoint()
            {
                LogLevel = Debugger.V2.Breakpoint.Types.LogLevel.Warning,
                LogMessageFormat = logMessageFormat,
                EvaluatedExpressions = { evaluatedExpressions },
            };

            LogEntry logEntry = new LogEntry
            {
                LogName = _logNameObj.ToString(),
                Severity = Logging.Type.LogSeverity.Warning,
                TextPayload = $"LOGPOINT: This is a log {evaluatedExpressions[0].Value}"
            };

            _client.WriteLogEntry(breakpoint);
            _mockLoggingClient.Verify(client =>
                client.WriteLogEntries(LogNameOneof.From(_logNameObj), _resource, null, new[] { logEntry }, null),
                Times.Once());
        }

        [Fact]
        public void WriteLogEntry_MessageWithComplexSubstitution()
        {
            string logMessageFormat = "I lost $$$0 today in the $1.";
            StackdriverVariable[] evaluatedExpressions = new StackdriverVariable[]
            {
                new StackdriverVariable()
                {
                    Value = "10000"
                },
                new StackdriverVariable()
                {
                    Value = "stock market"
                }
            };

            Debugger.V2.Breakpoint breakpoint = new Debugger.V2.Breakpoint()
            {
                LogLevel = Debugger.V2.Breakpoint.Types.LogLevel.Warning,
                LogMessageFormat = logMessageFormat,
                EvaluatedExpressions = { evaluatedExpressions },
            };

            LogEntry logEntry = new LogEntry
            {
                LogName = _logNameObj.ToString(),
                Severity = Logging.Type.LogSeverity.Warning,
                TextPayload = $"LOGPOINT: I lost ${evaluatedExpressions[0].Value} today in the {evaluatedExpressions[1].Value}."
            };

            _client.WriteLogEntry(breakpoint);
            _mockLoggingClient.Verify(client =>
                client.WriteLogEntries(LogNameOneof.From(_logNameObj), _resource, null, new[] { logEntry }, null),
                Times.Once());
        }

        [Fact]
        public void WriteLogEntry_MessageWithNestedMembers()
        {
            string logMessageFormat = "$0 and $1.";
            StackdriverVariable[] evaluatedExpressions = new StackdriverVariable[]
            {
                new StackdriverVariable()
                {
                    Members =
                    {
                        new StackdriverVariable
                        {
                            Name = "Key1",
                            Value = "Value1",
                            Type = "System.String"
                        },
                        new StackdriverVariable
                        {
                            Name = "Key2",
                            Value = "Value2",
                            Type = "System.String"
                        }
                    }
                },
                new StackdriverVariable()
                {
                    Members =
                    {
                        new StackdriverVariable
                        {
                            Name = "AnotherLevel",
                            Type = "Nested",
                            Members =
                            {
                                new StackdriverVariable
                                {
                                    Name = "Name",
                                    Value = "Nested",
                                    Type = "System.String"
                                }
                            }
                        }
                    }
                }
            };

            Debugger.V2.Breakpoint breakpoint = new Debugger.V2.Breakpoint()
            {
                LogLevel = Debugger.V2.Breakpoint.Types.LogLevel.Warning,
                LogMessageFormat = logMessageFormat,
                EvaluatedExpressions = { evaluatedExpressions },
            };

            LogEntry logEntry = new LogEntry
            {
                LogName = _logNameObj.ToString(),
                Severity = Logging.Type.LogSeverity.Warning,
                TextPayload = $"LOGPOINT: [ Key1 (System.String): Value1, Key2 (System.String): Value2] and [ AnotherLevel (Nested): [ Name (System.String): Nested]]."
            };

            _client.WriteLogEntry(breakpoint);
            _mockLoggingClient.Verify(client =>
                client.WriteLogEntries(LogNameOneof.From(_logNameObj), _resource, null, new[] { logEntry }, null),
                Times.Once());
        }
    }
}
