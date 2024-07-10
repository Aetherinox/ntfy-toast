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
#include "ntfytoasts.h"

typedef ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification *, ::IInspectable *>
        DesktopToastActivatedEventHandler;

typedef ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification *,
        ABI::Windows::UI::Notifications::ToastDismissedEventArgs *>
        DesktopToastDismissedEventHandler;

typedef ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification *,
        ABI::Windows::UI::Notifications::ToastFailedEventArgs *>
        DesktopToastFailedEventHandler;

class ToastEventHandler : public Microsoft::WRL::Implements<DesktopToastActivatedEventHandler,
                                                            DesktopToastDismissedEventHandler,
                                                            DesktopToastFailedEventHandler>
{

public:
    explicit ToastEventHandler::ToastEventHandler(const NtfyToasts &toast);
    ~ToastEventHandler();

    HANDLE event();
    NtfyToastActions::Actions &userAction();

    // DesktopToastActivatedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender,
                          _In_ IInspectable *args);

    // DesktopToastDismissedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender,
                          _In_ ABI::Windows::UI::Notifications::IToastDismissedEventArgs *e);

    // DesktopToastFailedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender,
                          _In_ ABI::Windows::UI::Notifications::IToastFailedEventArgs *e);

    // IUnknown
    IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_ref); }

    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG l = InterlockedDecrement(&m_ref);
        if (l == 0) {
            delete this;
        }
        return l;
    }

    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void **ppv)
    {
        if (IsEqualIID(riid, IID_IUnknown)) {
            *ppv = static_cast<IUnknown *>(static_cast<DesktopToastActivatedEventHandler *>(this));
        } else if (IsEqualIID(riid, __uuidof(DesktopToastActivatedEventHandler))) {
            *ppv = static_cast<DesktopToastActivatedEventHandler *>(this);
        } else if (IsEqualIID(riid, __uuidof(DesktopToastDismissedEventHandler))) {
            *ppv = static_cast<DesktopToastDismissedEventHandler *>(this);
        } else if (IsEqualIID(riid, __uuidof(DesktopToastFailedEventHandler))) {
            *ppv = static_cast<DesktopToastFailedEventHandler *>(this);
        } else {
            *ppv = nullptr;
        }

        if (*ppv) {
            reinterpret_cast<IUnknown *>(*ppv)->AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

private:
    ULONG m_ref;
    NtfyToastActions::Actions m_userAction;
    HANDLE m_event;
    const NtfyToasts &m_toast;
};
