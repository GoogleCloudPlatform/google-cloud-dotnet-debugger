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

#include "string_stream_wrapper.h"

#include <string>

#include "breakpoint.pb.h"

using google::cloud::diagnostics::debug::Status;
using google::cloud::diagnostics::debug::Variable;
using std::string;
using std::vector;

namespace google_cloud_debugger {

void SetErrorStatusMessage(Variable *variable, const std::string &err_string) {
  assert(variable != nullptr);

  std::unique_ptr<Status> status(new (std::nothrow) Status());
  status->set_message(err_string);
  status->set_iserror(true);
  variable->set_allocated_status(status.release());
}

void SetErrorStatusMessage(Variable *variable,
    StringStreamWrapper *string_stream) {
  assert(string_stream != nullptr);

  SetErrorStatusMessage(variable, string_stream->GetErrorString());
  string_stream->ResetErrorStream();
}

vector<WCHAR> ConvertStringToWCharPtr(const std::string &target_string) {
  if (target_string.size() == 0) {
    return vector<WCHAR>();
  }

#ifdef PAL_STDCPP_COMPAT
  // TODO(quoct): I cannot link the WideCharToMultiByte library on Linux.
  // If I add -lcoreclr or -lmscordaccore, then the linking works but
  // when running, the debugger will run into errors. Needs someone smarter
  // for this linking.
  // We feed MultiByteToWideChar a null terminated CHAR string and it will
  // tell us the size of the string so we can create a WCHAR string with the
  // same length.
  int string_size = target_string.size() + 1;
  vector<WCHAR> result(string_size, 0);
  for (size_t i = 0; i < target_string.size(); ++i) {
    result[i] = static_cast<WCHAR>(target_string[i]);
  }
  return result;
#else
  int string_size =
      MultiByteToWideChar(CP_UTF8, 0, target_string.c_str(), -1, nullptr, 0);
  if (string_size <= 0) {
    return vector<WCHAR>();
  }

  vector<WCHAR> result(string_size, 0);

  // MultiByteToWideChar should fill up vector result with converted WCHAR
  // string.
  string_size = MultiByteToWideChar(CP_UTF8, 0, target_string.c_str(), -1,
                                    result.data(), result.size());
  if (string_size <= 0) {
    return vector<WCHAR>();
  }
  return result;
#endif
}

string ConvertWCharPtrToString(const WCHAR *wchar_string) {
  if (!wchar_string || !(*wchar_string)) {
    return string();
  }

#ifdef PAL_STDCPP_COMPAT
  // TODO(quoct): I cannot link the WideCharToMultiByte library on Linux.
  // If I add -lcoreclr or -lmscordaccore, then the linking works but
  // when running, the debugger will run into errors. Needs someone smarter
  // for this linking.
  std::wstring temp_wstring;
  while (wchar_string && *wchar_string) {
    WCHAR current_char = *wchar_string;
    temp_wstring += static_cast<wchar_t>(current_char);
    ++wchar_string;
  }

  return string(temp_wstring.begin(), temp_wstring.end());
#else
  // We feeds WideCharToMultiByte a null terminated WCHAR string and it will
  // tells us the size of the string so we can create a CHAR string with the
  // same length.
  int string_size = WideCharToMultiByte(CP_UTF8, 0, wchar_string, -1, nullptr,
                                        0, nullptr, nullptr);

  string result(string_size, 0);
  // This time, WideCharToMultiByte will fill up the result buffer with
  // converted char.
  string_size = WideCharToMultiByte(CP_UTF8, 0, wchar_string, -1, &result[0],
                                    result.size(), nullptr, nullptr);
  if (string_size == 0) {
    return string();
  }

  // Minus one for the null-terminating character.
  result.pop_back();
  return result;
#endif
}

std::string ConvertWCharPtrToString(const vector<WCHAR> &wchar_vector) {
  return ConvertWCharPtrToString(wchar_vector.data());
}

}  // namespace google_cloud_debugger
