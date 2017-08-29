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

using System.Linq;
using Xunit;

namespace Google.Cloud.Diagnostics.Debug.Tests
{
    public class VariableExtensionsTests
    {
        private const string _name = "var-name";
        private const string _value = "var-value";
        private const string _type = "var-type";
        private const string _statusMessage = "status message";

        [Fact]
        public void Convert()
        {
            var variable = new Variable
            {
                Name = _name,
                Value = _value,
                Type = _type,
            };

            var sdVariable = variable.Convert();
            Assert.Equal(_name, sdVariable.Name);
            Assert.Equal(_value, sdVariable.Value);
            Assert.Equal(_type, sdVariable.Type);
            Assert.Empty(sdVariable.Members);
            Assert.Null(sdVariable.Status);
        }

        [Fact]
        public void Convert_Status()
        {
            var variable = new Variable
            {
                Name = _name,
                Value = _value,
                Type = _type,
                Status = new Status
                {
                    Message = _statusMessage,
                    Iserror = true,
                }
            };

            var sdVariable = variable.Convert();
            Assert.NotNull(sdVariable.Status);
            Assert.Equal(_statusMessage, sdVariable.Status.Description.Format);
            Assert.True(sdVariable.Status.IsError);
        }

        [Fact]
        public void Convert_Members()
        {
            var variable = new Variable
            {
                Name = _name,
                Value = _value,
                Type = _type,
                Members =
                {
                    new Variable
                    {
                        Name = _name + 1,
                        Value = _value + 1,
                        Type = _type + 1,
                    },
                    new Variable
                    {
                        Name = _name + 2,
                        Value = _value + 2,
                        Type = _type + 2,
                    },
                }
            };

            Assert.Equal(_name, variable.Name);
            Assert.Equal(_value, variable.Value);
            Assert.Equal(_type, variable.Type);
            Assert.Equal(2, variable.Members.Count);

            var variable1 = variable.Members.Where(v => v.Name.Equals(_name + 1)).Single();
            Assert.Equal(_name + 1, variable1.Name);
            Assert.Equal(_value + 1, variable1.Value);
            Assert.Equal(_type + 1, variable1.Type);
            Assert.Empty(variable1.Members);

            var variable2 = variable.Members.Where(v => v.Name.Equals(_name + 2)).Single();
            Assert.Equal(_name + 2, variable2.Name);
            Assert.Equal(_value + 2, variable2.Value);
            Assert.Equal(_type + 2, variable2.Type);
            Assert.Empty(variable2.Members);
        }

        [Fact]
        public void Convert_MembersDepth()
        {
            var variableOrig = new Variable();
            var variable = variableOrig;
            for (int i = 0; i < 5; i++)
            {
                variable.Name = _name + i;
                variable.Value = _value + i;
                variable.Type = _type + i;

                var newVariable = new Variable();
                variable.Members.Add(newVariable);
                variable = newVariable;
            }

            var sdVariable = variableOrig.Convert();
            for (int i = 0; i < 5; i++)
            {
                Assert.Equal(_name + i, sdVariable.Name);
                Assert.Equal(_value + i, sdVariable.Value);
                Assert.Equal(_type + i, sdVariable.Type);
                sdVariable = sdVariable.Members.Single();
            }
        }
    }
}
