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
#include <vector>

#include "breakpoint.pb.h"
#include "cor.h"

namespace google_cloud_debugger {

// This class is meant to be inherited and used for outputting error and
// output stream to the underlying ostringstream. It has methods to set the
// underlying streams as well as collecting the output and error stream.
// This class is NOT thread-safe.
class StringStreamWrapper {
 public:
  // Sets error_stream_ to error_stream and returns the old error stream.
  std::ostringstream *SetErrorStream(std::ostringstream *error_stream) {
    std::ostringstream *old_stream = error_stream_.release();
    error_stream_.reset(error_stream);
    return old_stream;
  }

  // Writes the string error to the error_stream_.
  void WriteError(const std::string &error) {
    *error_stream_ << error << std::endl;
  }

  // Gets the underlying error stream.
  std::ostringstream *GetErrorStream() { return error_stream_.get(); }

  // Gets string collected in the error stream.
  std::string GetErrorString() { return error_stream_->str(); }

  // Resets the error stream.
  void ResetErrorStream() {
    error_stream_->str("");
    error_stream_->clear();
  }

 private:
  // The underlying error stream.
  std::unique_ptr<std::ostringstream> error_stream_ =
      std::unique_ptr<std::ostringstream>(new (std::nothrow)
                                              std::ostringstream());
};

// Sets the Status field of variable using error string err_string.
void SetErrorStatusMessage(google::cloud::diagnostics::debug::Variable *var,
                           const std::string &err_string);

// Sets the Status field of variable using error string from string_stream
// object. This will reset the error stream of string_stream afterwards.
void SetErrorStatusMessage(google::cloud::diagnostics::debug::Variable *var,
                           StringStreamWrapper *string_stream);

// Sets the Status field of breakpoint using error string err_string.
void SetErrorStatusMessage(
    google::cloud::diagnostics::debug::Breakpoint *breakpoint,
    const std::string &err_string);

// Helper function to convert a string to null-terminated WCHAR vector.
// If target_string is empty or if there are failures,
// this function returns an empty vector.
std::vector<WCHAR> ConvertStringToWCharPtr(const std::string &target_string);

// Helper function to convert a null-terminated WCHAR array to string.
// If wchar_string is null or if there are failures,
// this function returns an empty string.
std::string ConvertWCharPtrToString(const WCHAR *wchar_string);

// ConvertWCharPtrToString functions that takes in a vector
// instead of WCHAR array.
std::string ConvertWCharPtrToString(const std::vector<WCHAR> &wchar_vector);

}  // namespace google_cloud_debugger

#endif
