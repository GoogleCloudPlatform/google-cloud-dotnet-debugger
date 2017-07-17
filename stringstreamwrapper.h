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

#ifndef STRING_STREAM_WRAPPER_H_
#define STRING_STREAM_WRAPPER_H_

#include <memory>
#include <sstream>
#include <string>

namespace google_cloud_debugger {

// This class is meant to be inherited and used for outputting error and
// output stream to the underlying ostringstream. It has methods to set the
// underlying streams as well as collecting the output and error stream.
// This class is NOT thread-safe.
class StringStreamWrapper {
 public:
  // Sets output_stream_ to output_stream and returns the old output stream.
  std::ostringstream *SetOutputStream(std::ostringstream *output_stream) {
    std::ostringstream *old_stream = output_stream_.release();
    output_stream_.reset(output_stream);
    return old_stream;
  }

  // Sets error_stream_ to error_stream and returns the old error stream.
  std::ostringstream *SetErrorStream(std::ostringstream *error_stream) {
    std::ostringstream *old_stream = error_stream_.release();
    error_stream_.reset(error_stream);
    return old_stream;
  }

  // Writes the string output to the output_stream_.
  void WriteOutput(const std::string &output) { *output_stream_ << output; }

  // Writes the string error to the error_stream_.
  void WriteError(const std::string &error) { *error_stream_ << error; }

  // Gets the underlying error stream.
  std::ostringstream *GetErrorStream() { return error_stream_.get(); }

  // Gets the underlying output stream.
  std::ostringstream *GetOutputStream() { return output_stream_.get(); }

  // Gets string collected in the error stream.
  std::string GetErrorString() { return error_stream_->str(); }

  // Gets the string collected in the output stream.
  std::string GetOutputString() { return output_stream_->str(); }

  // Resets the error stream.
  void ResetErrorStream() {
    error_stream_->str("");
    error_stream_->clear();
  }

  // Resets the output stream.
  void ResetOutputStream() {
    output_stream_->str("");
    output_stream_->clear();
  }

  // Returns a JSON error status message in the form:
  // "status": {
  //   "isError": true,
  //   "description": {
  //     "format": "<The error string>"
  //   }
  //  }
  std::string GetErrorStatusMessage() {
    std::string status_message =
      "\"status\": { \"isError\": true, \"description\": { \"format\": \"";
    status_message += GetErrorString();
    status_message += "\" } }";
    return status_message;
  }

 private:
  // The underlying output stream.
  std::unique_ptr<std::ostringstream> output_stream_ =
      std::unique_ptr<std::ostringstream>(new (std::nothrow)
                                              std::ostringstream());

  // The underlying error stream.
  std::unique_ptr<std::ostringstream> error_stream_ =
      std::unique_ptr<std::ostringstream>(new (std::nothrow)
                                              std::ostringstream());
};

}  // namespace google_cloud_debugger

#endif
