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

using Google.Api.Gax;
using Google.Cloud.DevTools.Source.V1;
using System;
using System.Collections.Generic;
using System.IO;
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class AgentOptionsTests
    {
        private const string _projectId = "pid";
        private const string _module = "module";
        private const string _version = "version";
        private static readonly string _debugger = Path.GetTempFileName();
        private static readonly string _application = Path.GetTempFileName();
        private static readonly string _sourceContext = Path.GetTempFileName();

        private static readonly string[] _args = new string[]
        {
            $"--module={_module}",
            $"--version={_version}",
            $"--project-id={_projectId}",
            $"--debugger={_debugger}",
            $"--application-start-command={_application}",
        };

        private static readonly GaePlatformDetails _gaeDetails = new GaePlatformDetails(
            _projectId, "instance", _module, _version);
        private static readonly Platform _gaePlatform = new Platform(_gaeDetails);
        private static readonly Platform _unknownPlatform = new Platform();

        [Fact]
        public void Parse()
        {
            var options = AgentOptions.Parse(_args);
            Assert.Equal(_module, options.Module);
            Assert.Equal(_version, options.Version);
            Assert.Equal(_debugger, options.Debugger);
            Assert.Equal(_application, options.ApplicationStartCommand);
            Assert.Equal(_projectId, options.ProjectId);
            Assert.Null(options.ApplicationId);
            Assert.Null(options.SourceContextFile);
            Assert.Null(options.SourceContext);
            Assert.False(options.PropertyEvaluation);
        }

        [Fact]
        public void Parse_Missing_Debugger_File()
        {
            var args = new List<string>(_args);
            args[3] = "--debugger=invalid";
            Assert.Throws<FileNotFoundException>(() => AgentOptions.Parse(args.ToArray()));
        }
            
        [Fact]
        public void Parse_Multiple_Applications()
        {
            var args = new List<string>(_args);
            args.Add("--application-id=12345");
            Assert.Throws<ArgumentException>(() => AgentOptions.Parse(args.ToArray()));
        }

        [Fact]
        public void Parse_SourceContext()
        {
            var sourceContext = new SourceContext
            {
                Git = new GitSourceContext
                {
                    Url = "some-url.com",
                    RevisionId = "rev-id",
                }
            };
            var filePath = Path.GetTempFileName();
            File.WriteAllText(filePath, sourceContext.ToString());

            var args = new List<string>(_args);
            args.Add($"--source-context={filePath}");
            var options = AgentOptions.Parse(args.ToArray());

            Assert.Equal(sourceContext, options.SourceContext);
        }

        [Fact]
        public void GetDebugger_Env() =>
            TestEnvVariable(() => AgentOptions.GetDebugger(), AgentOptions.DebuggerEnvironmentVariable, "some-path/to/a/debugger");
        

        [Fact]
        public void GetDebugger_Null() => Assert.Null(AgentOptions.GetDebugger());

        [Fact]
        public void GetProject_Platform() => 
            Assert.Equal(_projectId, AgentOptions.GetProject(_gaePlatform));

        [Fact]
        public void GetProject_Env() => 
            TestEnvVariable(() => AgentOptions.GetProject(), AgentOptions.ProjectEnvironmentVariable, "pid-2");

        [Fact]
        public void GetProject_Null() => Assert.Null(AgentOptions.GetProject(_unknownPlatform));

        [Fact]
        public void GetModule_Platform() =>
            Assert.Equal(_module, AgentOptions.GetModule(_gaePlatform));

        [Fact]
        public void GetModule_Env() =>
            TestEnvVariable(() => AgentOptions.GetModule(), AgentOptions.ModuleEnvironmentVariable, "service-2");

        [Fact]
        public void GetModule_Null() => Assert.Null(AgentOptions.GetModule(_unknownPlatform));

        [Fact]
        public void GetVersion_Platform() =>
            Assert.Equal(_version, AgentOptions.GetVersion(_gaePlatform));

        [Fact]
        public void GetVersion_Env() =>
            TestEnvVariable(() => AgentOptions.GetVersion(), AgentOptions.VersionEnvironmentVariable, "version-2");

        [Fact]
        public void GetVersion_Null() => Assert.Null(AgentOptions.GetVersion(_unknownPlatform));

        [Fact]
        public void GetSourceContextFile_Param() => 
            Assert.Equal(_sourceContext, AgentOptions.GetSourceContextFile(_sourceContext));
        

        [Fact]
        public void GetSourceContextFile_Env() => 
            TestEnvVariable(() => AgentOptions.GetSourceContextFile(null), 
                AgentOptions.SourceContextEnvironmentVariable, _sourceContext);
        

        [Fact]
        public void GetSourceContextFile_App()
        {
            var filePath = Path.Combine(AppContext.BaseDirectory, AgentOptions.SourceContextFileName);
            try
            {
                File.Create(filePath).Dispose();
                Assert.Equal(filePath, AgentOptions.GetSourceContextFile(null));
            }
            finally
            {
                File.Delete(filePath);
            }
        }

        [Fact]
        public void GetSourceContextFile_Null() => Assert.Null(AgentOptions.GetSourceContextFile(null));

        /// <summary>
        /// Sets the <paramref name="envVar"/> to <paramref name="envVal"/> and checks that
        /// the <paramref name="func"/> returns the <paramref name="envVal"/>.
        /// </summary>
        private void TestEnvVariable(Func<string> func, string envVar, string envVal)
        {
            try
            {
                Environment.SetEnvironmentVariable(envVar, envVal, EnvironmentVariableTarget.Process);
                Assert.Equal(envVal, func());
            }
            finally
            {
                Environment.SetEnvironmentVariable(envVar, null, EnvironmentVariableTarget.Process);
            }
        }
    }
}
