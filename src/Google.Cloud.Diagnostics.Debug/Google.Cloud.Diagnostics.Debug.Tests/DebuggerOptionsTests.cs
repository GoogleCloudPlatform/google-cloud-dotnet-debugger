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
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class DebuggerOptionsTests
    {
        private readonly string _startCmd = "start-an-app";
        private readonly int _processId = 12345;

        [Fact]
        public void FromAgentOptions()
        {
            var agentOptions = new AgentOptions
            {
                ApplicationId = _processId,
            };
            var options = DebuggerOptions.FromAgentOptions(agentOptions);

            Assert.False(options.PropertyEvaluation);
            Assert.False(options.ConditionEvaluation);
            Assert.Null(options.ApplicationStartCommand);
            Assert.Equal(_processId, options.ApplicationId);
            Assert.StartsWith(Constants.PipeName, options.PipeName);
        }

        [Fact]
        public void FromAgentOptions_PropertyEval()
        {
            var agentOptions = new AgentOptions
            {
                ApplicationStartCommand = _startCmd,
                PropertyEvaluation = true
            };
            var options = DebuggerOptions.FromAgentOptions(agentOptions);

            Assert.False(options.ConditionEvaluation);
            Assert.True(options.PropertyEvaluation);
            Assert.Null(options.ApplicationId);
            Assert.Equal(_startCmd, options.ApplicationStartCommand);
            Assert.StartsWith(Constants.PipeName, options.PipeName);
        }

        [Fact]
        public void FromAgentOptions_ConditionEval()
        {
            var agentOptions = new AgentOptions
            {
                ApplicationStartCommand = _startCmd,
                ConditionEvaluation = true
            };
            var options = DebuggerOptions.FromAgentOptions(agentOptions);

            Assert.False(options.PropertyEvaluation);
            Assert.True(options.ConditionEvaluation);
            Assert.Null(options.ApplicationId);
            Assert.Equal(_startCmd, options.ApplicationStartCommand);
            Assert.StartsWith(Constants.PipeName, options.PipeName);
        }

        [Fact]
        public void FromAgentOptionsThrows_None() =>
            Assert.Throws<ArgumentException>(() => DebuggerOptions.FromAgentOptions(new AgentOptions()));

        [Fact]
        public void FromAgentOptionsThrows_Both()
        {
            var agentOptions = new AgentOptions
            {
                ApplicationId = _processId,
                ApplicationStartCommand = _startCmd
            };
            Assert.Throws<ArgumentException>(() => DebuggerOptions.FromAgentOptions(agentOptions));
        }

        [Fact]
        public void ToString_ApplicationId()
        {
            var agentOptions = new AgentOptions
            {
                ApplicationId = _processId,
                PropertyEvaluation = true,
                ConditionEvaluation = true,
            };
            var options = DebuggerOptions.FromAgentOptions(agentOptions);
            var optionsString = options.ToString();

            Assert.Contains($"{DebuggerOptions.PipeNameOption}={Constants.PipeName}", optionsString);
            Assert.Contains($"{DebuggerOptions.ApplicationIdOption}={_processId}", optionsString);
            Assert.Contains($"{DebuggerOptions.PropertyEvaluationOption}", optionsString);
            Assert.Contains($"{DebuggerOptions.ConditionEvaluationOption}", optionsString);
            Assert.DoesNotContain(DebuggerOptions.ApplicationStartCommandOption, optionsString);
        }

        [Fact]
        public void ToString_StartCommand()
        {
            var agentOptions = new AgentOptions
            {
                ApplicationStartCommand = _startCmd,
            };
            var options = DebuggerOptions.FromAgentOptions(agentOptions);
            var optionsString = options.ToString();
            Assert.Contains($"{DebuggerOptions.PipeNameOption}={Constants.PipeName}", optionsString);
            Assert.Contains($"{DebuggerOptions.ApplicationStartCommandOption}=\"{_startCmd}\"", optionsString);
            Assert.DoesNotContain(DebuggerOptions.PropertyEvaluationOption, optionsString);
            Assert.DoesNotContain(DebuggerOptions.ConditionEvaluationOption, optionsString);
            Assert.DoesNotContain(DebuggerOptions.ApplicationIdOption, optionsString);
        }
    }
}
