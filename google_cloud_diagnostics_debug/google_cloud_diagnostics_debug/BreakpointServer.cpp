#ifdef _WIN32
#ifndef NAMED_PIPE_CLIENT_H_
#define NAMED_PIPE_CLIENT_H_

#include <iostream>
#include <cor.h>
#include <string>
#include <windef.h>

#include "constants.h"
#include "i_named_pipe.h"

namespace google_cloud_debugger {

    // A named pipe client for windows.
    class NamedPipeClient : public INamedPipe {
    public:
        ~NamedPipeClient();
        HRESULT Initialize() override;
        HRESULT WaitForConnection() override;
        HRESULT ReadMessage(CHAR** bytes) override;
        HRESULT WriteMessage(const std::string &message) override;

    private:
        // The name of the pipe.
        const std::wstring pipe_name_ = std::wstring(L"\\\\.\\pipe\\") + std::wstring(kPipeNameW);

        // A handle to the open pipe.
        HANDLE pipe_ = INVALID_HANDLE_VALUE;
    };

}  // namespace google_cloud_debugger

#endif  //  NAMED_PIPE_CLIENT_H_
#endif  //  _WIN32