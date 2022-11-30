// Copyright 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define xstr(s) str(s)
#define str(s) #s

#ifndef CLANG_CL
#define CLANG_CL clang-cl.exe
#endif

#ifndef CCACHE
#define CCACHE sccache.exe
#endif

using ParamList = std::vector<std::wstring>;

constexpr size_t klpCommandLineMax = 32767;

std::wstring read_file(const std::wstring& file) {
    auto hin = CreateFile(file.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, 0);
    if (hin == INVALID_HANDLE_VALUE) {
        return L"";
    }

    wchar_t rspBuffer[klpCommandLineMax];
    char fileBuf[klpCommandLineMax * sizeof(wchar_t)];
    DWORD lpNumberOfBytesRead = 0;
    if (!ReadFile(hin, fileBuf, sizeof(fileBuf), &lpNumberOfBytesRead, NULL)) {
        return L"";
    }
    size_t size = 0;
    if (mbstowcs_s(&size, rspBuffer, sizeof(rspBuffer), fileBuf,
                   lpNumberOfBytesRead) != 0) {
        return L"";
    }
    return std::wstring(rspBuffer, size - 1);
}

ParamList extract_response_files(const ParamList& params) {
    ParamList flattened;
    for (const auto& param : params) {
        if (param.at(0) != '@') {
            flattened.push_back(param);
            continue;
        }

        // Found a response file!
        auto file_name = param.substr(1);

        // Unable to open response file, do not flatten it.
        auto text = read_file(file_name);
        if (text.empty()) {
            flattened.push_back(param);
            continue;
        }

        // Read the response file, and append the parameters to our
        // flattened list.
        flattened.push_back(text);
    }

    return flattened;
}

std::wstring formatLastErr() {
    DWORD error = GetLastError();
    if (error) {
        constexpr size_t max_str_len = 512;
        wchar_t lpMsgBuf[max_str_len];
        DWORD bufLen = FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                lpMsgBuf, max_str_len, nullptr);
        if (bufLen) {
            std::wstring result(lpMsgBuf, lpMsgBuf + bufLen);
            return result;
        }
    }
    return L"";
}

int wmain(int argc, wchar_t* argv[], wchar_t* envp[]) {
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    ParamList params;
    params.push_back(L"" xstr(CCACHE));
    params.push_back(L"" xstr(CLANG_CL));
    if (argc < 2) {
        std::wcerr
                << "This flattens every rsp file (parameters starting with @), "
                   "by reading the rsp file and extracting the parameters to "
                   "pass them to the executable. "
                << std::endl
                << "This is mainly a workaround "
                   "to limitation of the number of characters that can be used "
                   "in cmd (8192) vs create process (32767)."
                << std::endl;
        return -1;
    }

    for (int i = 1; i < argc; i++) {
        params.push_back(argv[i]);
    }

    ParamList flattened = extract_response_files(params);
    std::wstring arguments;  
    for (const auto& param : flattened) {
        arguments += param + L" ";
    }

    // The maximum length of this string is 32,767 characters, including the
    // Unicode terminating null character. If lpApplicationName is NULL, the
    // module name portion of lpCommandLine is limited to MAX_PATH characters.
    if (arguments.size() >= klpCommandLineMax) {
        // Do not flatten the @rsp..
        arguments.clear();
        for (const auto& param : flattened) {
            arguments += param + L" ";
        }
    }

    LPWSTR args = (LPWSTR)arguments.c_str();
    if (!CreateProcess(0, args, 0, 0, true, 0, 0, 0, &si, &pi)) {
        std::wcerr << "Failed to start " << arguments << " due to "
                   << formatLastErr() << std::endl;
        return -1;
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Process has exited - check its exit code
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode;
}