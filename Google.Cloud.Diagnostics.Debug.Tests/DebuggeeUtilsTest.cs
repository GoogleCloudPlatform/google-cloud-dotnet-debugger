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
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class DebuggeeUtilsTest
    {
        private const string _projectId = "project-id";
        private const string _module = "test-app";
        private const string _version = "1.0.4";

        [Fact]
        public void CreateDebuggee()
        {
            var debuggee = DebuggeeUtils.CreateDebuggee(_projectId, _module, _version);
            Assert.Equal(DebuggeeUtils.GetAgentVersion(Common.Platform), debuggee.AgentVersion);
            Assert.Equal(DebuggeeUtils.GetDescription(_module, _version), debuggee.Description);
            Assert.Equal(_projectId, debuggee.Project);
            Assert.Equal(3, debuggee.Labels.Count);
            Assert.NotNull(debuggee.SourceContexts);
            Assert.False(string.IsNullOrWhiteSpace(debuggee.Uniquifier));
        }

        [Fact]
        public void GetUniquifier()
        {
            var debuggee = DebuggeeUtils.CreateDebuggee(_projectId, _module, _version);
            var uniquifier = DebuggeeUtils.GetUniquifier(debuggee);
            Assert.False(string.IsNullOrWhiteSpace(uniquifier));
            Assert.Equal(DebuggeeUtils.GetUniquifier(debuggee), uniquifier);
        }

        [Fact]
        public void GetUniquifierNotEqual()
        {
            var debuggee = DebuggeeUtils.CreateDebuggee(_projectId, _module, _version);
            var uniquifier = DebuggeeUtils.GetUniquifier(debuggee);

            var debuggee2 = DebuggeeUtils.CreateDebuggee(_projectId, _module, "1.0.5");
            var uniquifier2 = DebuggeeUtils.GetUniquifier(debuggee2);

            Assert.NotEqual(uniquifier2, uniquifier);
        }

        [Fact]
        public void GetDescription()
        {
            Assert.Equal($"{_module} - {_version}", DebuggeeUtils.GetDescription(_module, _version));
            Assert.Equal(_module, DebuggeeUtils.GetDescription(_module, null));
            Assert.Equal(_module, DebuggeeUtils.GetDescription(_module, ""));
        }

        [Fact]
        public void GetLabels()
        {
            var labels = DebuggeeUtils.GetLabels(_projectId, _module, _version);
            Assert.Equal(3, labels.Count);
            Assert.Equal(_projectId, labels["projectid"]);
            Assert.Equal(_module, labels["module"]);
            Assert.Equal(_version, labels["version"]);
        }

        [Fact]
        public void GetLabelsNoVersion()
        {
            var labels = DebuggeeUtils.GetLabels(_projectId, _module, null);
            Assert.Equal(2, labels.Count);
            Assert.Equal(_projectId, labels["projectid"]);
            Assert.Equal(_module, labels["module"]);
        }

        [Fact]
        public void GetVersion()
        {
            Assert.False(string.IsNullOrWhiteSpace(DebuggeeUtils.GetVersion()));
        }

        [Fact]
        public void GetAgentVersion()
        {
            var platform = new Platform(new GaePlatformDetails(_projectId, "instance", _module, _version));
            var agentVersion = DebuggeeUtils.GetAgentVersion(platform);
            Assert.Contains("google.com", agentVersion);
            Assert.Contains("csharp", agentVersion);
            Assert.Contains("gae", agentVersion);
            Assert.Contains(DebuggeeUtils.GetVersion(), agentVersion);
        }

        [Fact]
        public void GetPlatform()
        {
            var gae = new Platform(new GaePlatformDetails("", "", "", ""));
            Assert.Equal("gae", DebuggeeUtils.GetPlatform(gae));

            var gce = new Platform(new GcePlatformDetails("", "", "", ""));
            Assert.Equal("gce", DebuggeeUtils.GetPlatform(gce));

            var gke = new Platform(new GkePlatformDetails("", "", "", "", ""));
            Assert.Equal("gke", DebuggeeUtils.GetPlatform(gke));

            var unknown = new Platform();
            Assert.Null(DebuggeeUtils.GetPlatform(unknown));
        }
    }
}
