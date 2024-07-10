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

#pragma once

#include <comdef.h>
#include <filesystem>
#include <sstream>
#include <unordered_map>

class ToastLog;

class ToastLog
{
public:
    ToastLog();
    ~ToastLog();

    inline ToastLog &log() { return *this; }

private:
    std::wstringstream m_log;
    template<typename T>
    friend ToastLog &operator<<(ToastLog &, const T &);
};

template<typename T>
ToastLog &operator<<(ToastLog &log, const T &t)
{
    log.m_log << L" " << t;

    return log;
}

template<>
inline ToastLog &operator<<(ToastLog &log, const HRESULT &hr)
{
    if (FAILED(hr)) {
        _com_error err(hr);
        log.m_log << L" Error: " << hr << L" " << err.ErrorMessage();
    }

    return log;
}

#define tLog ToastLog().log() << __FUNCSIG__ << L"\n\t\t"

namespace Utils {
bool registerActivator();
void unregisterActivator();

std::unordered_map<std::wstring_view, std::wstring_view> splitData(const std::wstring_view &data);

const std::filesystem::path &selfLocate();

std::wstring formatData(const std::vector<std::pair<std::wstring_view, std::wstring_view>> &data);

bool writePipe(const std::filesystem::path &pipe, const std::wstring &data, bool wait = false);
bool startProcess(const std::filesystem::path &app);

inline bool checkResult(const char *file, const long line, const char *func, const HRESULT &hr)
{
    if (FAILED(hr)) {
        tLog << file << line << func << L":\n\t\t\t" << hr;
        return false;
    }

    return true;
}

std::wstring formatWinError(unsigned long errorCode);
};

#define ST_CHECK_RESULT(hr) Utils::checkResult(__FILE__, __LINE__, __FUNCSIG__, hr)

#define ST_RETURN_ON_ERROR(hr)                                                                     \
    do {                                                                                           \
        HRESULT _tmp = hr;                                                                         \
        if (!ST_CHECK_RESULT(_tmp)) {                                                              \
            return _tmp;                                                                           \
        }                                                                                          \
    } while (false)
