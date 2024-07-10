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

#include <algorithm>
#include <ntverp.h>
#include <sstream>
#include <wrl.h>

#define ST_WSTRINGIFY(X) L##X
#define ST_STRINGIFY(X) ST_WSTRINGIFY(X)

typedef struct NOTIFICATION_USER_INPUT_DATA
{
    LPCWSTR Key;
    LPCWSTR Value;
} NOTIFICATION_USER_INPUT_DATA;

MIDL_INTERFACE("53E31837-6600-4A81-9395-75CFFE746F94")
INotificationActivationCallback : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Activate(
            __RPC__in_string LPCWSTR appUserModelId, __RPC__in_opt_string LPCWSTR invokedArgs,
            __RPC__in_ecount_full_opt(count) const NOTIFICATION_USER_INPUT_DATA *data,
            ULONG count) = 0;
};

// The COM server which implements the callback notifcation from Action Center
class DECLSPEC_UUID(NTFYTOAST_CALLBACK_GUID) NtfyToastActionCenterIntegration
    : public Microsoft::WRL::RuntimeClass<
              Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
              INotificationActivationCallback>
{
public:
    static std::wstring uuid()
    {
        return ST_STRINGIFY(NTFYTOAST_CALLBACK_GUID);
    }

    NtfyToastActionCenterIntegration() {}
    virtual HRESULT STDMETHODCALLTYPE Activate(__RPC__in_string LPCWSTR appUserModelId,
                                               __RPC__in_opt_string LPCWSTR invokedArgs,
                                               __RPC__in_ecount_full_opt(count)
                                                       const NOTIFICATION_USER_INPUT_DATA *data,
                                               ULONG count) override
    {
        if (invokedArgs == nullptr) {
            return S_OK;
        }
        std::wstringstream msg;
        for (ULONG i = 0; i < count; ++i) {
            std::wstring tmp = data[i].Value;

            std::replace(tmp.begin(), tmp.end(), L'\r', L'\n');
            msg << tmp;
        }

        return NtfyToasts::backgroundCallback(appUserModelId, invokedArgs, msg.str());
    }
};

CoCreatableClass(NtfyToastActionCenterIntegration);
