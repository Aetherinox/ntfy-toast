/*
    Copyright 2024-2024 Aetherinox
    Copyright 2013-2019 Hannah von Reth <vonreth@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "utils.h"
#include "ntfytoasts.h"

#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/module.h>

using namespace Microsoft::WRL;

namespace {
    bool s_registered = false;
}

namespace Utils {

bool registerActivator()
{
    if (!s_registered) {
        s_registered = true;
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::Create([] {});
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().IncrementObjectCount();

        return SUCCEEDED(
                Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().RegisterObjects());
    }
    return true;
}

void unregisterActivator()
{
    if (s_registered) {
        s_registered = false;
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().UnregisterObjects();
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().DecrementObjectCount();
    }
}

std::unordered_map<std::wstring_view, std::wstring_view> splitData(const std::wstring_view &data)
{
    std::unordered_map<std::wstring_view, std::wstring_view> out;
    size_t start = 0;
    for (size_t end = data.find(L";", start); end != std::wstring::npos;
        start = end + 1, end = data.find(L";", start)) {

        if (start == end) {
            end = data.size();
        }

        const std::wstring_view tmp(data.data() + start, end - start);
        const auto pos = tmp.find(L"=");

        if (pos > 0) {
            out[tmp.substr(0, pos)] = tmp.substr(pos + 1);
        }

        // tLog << L"'" << tmp.substr(0, pos) << L"' = '" << tmp.substr(pos + 1) << L"'";
    }
    return out;
}

const std::filesystem::path &selfLocate()
{
    static const std::filesystem::path path = [] {
        // don't modify the lasterror
        const auto lastError = GetLastError();
        std::wstring buf;
        size_t size;

        do {
            buf.resize(buf.size() + 1024);
            size = GetModuleFileNameW(nullptr, const_cast<wchar_t *>(buf.data()),
                                      static_cast<DWORD>(buf.size()));
        } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        buf.resize(size);
        SetLastError(lastError);

        return buf;

    }();

    return path;
}

bool writePipe(const std::filesystem::path &pipe, const std::wstring &data, bool wait)
{
    if (wait) {
        WaitNamedPipe(pipe.wstring().c_str(), 20000);
    }

    HANDLE hPipe = CreateFile(pipe.wstring().c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0,
                              nullptr);

    if (hPipe != INVALID_HANDLE_VALUE) {
        DWORD written;
        const DWORD toWrite = static_cast<DWORD>(data.size() * sizeof(wchar_t));
        WriteFile(hPipe, data.c_str(), toWrite, &written, nullptr);
        const bool success = written == toWrite;
        tLog << (success ? L"Wrote: " : L"Failed to write: ") << data << " to " << pipe;
        WriteFile(hPipe, nullptr, sizeof(wchar_t), &written, nullptr);
        CloseHandle(hPipe);

        return success;
    }

    tLog << L"Failed to open pipe: " << pipe << L" data: " << data;

    return false;
}

bool startProcess(const std::filesystem::path &app)
{
    STARTUPINFO info = {};
    info.cb = sizeof(info);
    PROCESS_INFORMATION pInfo = {};
    const auto application = app.wstring();

    if (!CreateProcess(const_cast<wchar_t *>(application.c_str()),
                       const_cast<wchar_t *>(application.c_str()), nullptr, nullptr, false,
                       DETACHED_PROCESS | INHERIT_PARENT_AFFINITY | CREATE_NO_WINDOW, nullptr,
                       nullptr, &info, &pInfo)) {
        tLog << L"Failed to start: " << app;

        return false;
    }

    WaitForInputIdle(pInfo.hProcess, INFINITE);
    DWORD status;
    GetExitCodeProcess(pInfo.hProcess, &status);
    CloseHandle(pInfo.hProcess);
    CloseHandle(pInfo.hThread);

    tLog << L"Started: " << app << L" Status: "
         << (status == STILL_ACTIVE ? L"STILL_ACTIVE" : std::to_wstring(status));

    return status == STILL_ACTIVE;
}

std::wstring formatData(const std::vector<std::pair<std::wstring_view, std::wstring_view>> &data)
{
    std::wstringstream out;
    const auto add = [&](const std::pair<std::wstring_view, std::wstring_view> &p) {
        if (!p.second.empty()) {
            out << p.first << L"=" << p.second << L";";
        }
    };

    for (const auto &p : data) {
        add(p);
    }

    add({ L"version", NtfyToasts::version() });

    return out.str();
}

std::wstring formatWinError(unsigned long errorCode)
{
    wchar_t *error = nullptr;
    size_t len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                               nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               reinterpret_cast<LPWSTR>(&error), 0, nullptr);

    const auto out = std::wstring(error, len);
    LocalFree(error);

    return out;
}
}

ToastLog::ToastLog()
{
    *this << Utils::selfLocate() << L"v" << NtfyToasts::version() << L"\n\t";
}

ToastLog::~ToastLog()
{
    m_log << L"\n";
    OutputDebugStringW(m_log.str().c_str());
}
