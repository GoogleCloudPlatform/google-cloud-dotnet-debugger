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
using System.Linq;
using StackdriverVariable = Google.Cloud.Debugger.V2.Variable;
using StackdriverStatusMessage = Google.Cloud.Debugger.V2.StatusMessage;

namespace Google.Cloud.Diagnostics.Debug
{
    /// <summary>
    /// Extensions for <see cref="Variable"/>s and <see cref="StackdriverVariable"/>s.
    /// </summary>
    internal static class VariableExtensions
    {
        /// <summary>
        /// Converts a <see cref="Variable"/> to a <see cref="StackdriverVariable"/>.
        /// </summary>
        public static StackdriverVariable Convert(this Variable variable)
        {
            GaxPreconditions.CheckNotNull(variable, nameof(variable));
            var newVariable = new StackdriverVariable
            {
                Name = variable.Name,
                Value = variable.Value,
                Type = variable.Type,
                // TODO(talarico): Look into making sure we don't have cycles
                Members = { variable.Members?.Select(x => x.Convert()).ToList() }
            };

            if (variable.Members != null)
            {
                newVariable.Members.AddRange(variable.Members.Select(x => x.Convert()));
            }

            if (variable.Status != null)
            {
                newVariable.Status = new StackdriverStatusMessage
                {
                    IsError = newVariable.Status.IsError,
                    Description =
                    {
                        Format = variable.Status.Message
                    }
                };
            }
            return newVariable;
        }
    }
}
