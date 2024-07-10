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

#include "ntfytoasts.h"
#include "toasteventhandler.h"
#include "utils.h"

#include <sstream>
#include <iostream>
#include <wchar.h>
#include <algorithm>
#include <assert.h>

using namespace ABI::Windows::UI::Notifications;

ToastEventHandler::ToastEventHandler(const NtfyToasts &toast)
    : m_ref(1), m_userAction(NtfyToastActions::Actions::Hidden), m_toast(toast)
{
    std::wstringstream eventName;
    eventName << L"ToastEvent" << m_toast.id();
    m_event = CreateEventW(nullptr, true, false, eventName.str().c_str());
}

ToastEventHandler::~ToastEventHandler()
{
    CloseHandle(m_event);
}

HANDLE ToastEventHandler::event()
{
    return m_event;
}

NtfyToastActions::Actions &ToastEventHandler::userAction()
{
    return m_userAction;
}

// DesktopToastActivatedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification * /*sender*/,
                                         _In_ IInspectable *args)
{
    IToastActivatedEventArgs *buttonReply = nullptr;
    args->QueryInterface(&buttonReply);
    if (buttonReply == nullptr) {
        std::wcerr << L"args is not a IToastActivatedEventArgs" << std::endl;
    } else {
        HSTRING args;
        buttonReply->get_Arguments(&args);
        std::wstring data = WindowsGetStringRawBuffer(args, nullptr);
        tLog << data;

        const auto dataMap = Utils::splitData(data);
        const auto action = NtfyToastActions::getAction(dataMap.at(L"action"));
        assert(dataMap.at(L"notificationId") == m_toast.id());

        if (action == NtfyToastActions::Actions::TextEntered) {
            // The text is only passed to the named pipe
            tLog << L"The user entered a text.";
            m_userAction = NtfyToastActions::Actions::TextEntered;
        } else if (action == NtfyToastActions::Actions::Clicked) {
            tLog << L"The user clicked on the toast.";
            m_userAction = NtfyToastActions::Actions::Clicked;
        } else {
            tLog << L"The user clicked on a toast button.";
            std::wcout << dataMap.at(L"button") << std::endl;
            m_userAction = NtfyToastActions::Actions::ButtonClicked;
        }
        if (m_toast.useFalbackMode() && !m_toast.pipeName().empty()) {
            Utils::writePipe(m_toast.pipeName(), m_toast.formatAction(m_userAction));
        }
    }

    SetEvent(m_event);

    return S_OK;
}

// DesktopToastDismissedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification * /* sender */,
                                         _In_ IToastDismissedEventArgs *e)
{
    ToastDismissalReason tdr;
    HRESULT hr = e->get_Reason(&tdr);

    if (SUCCEEDED(hr)) {
        switch (tdr) {

        case ToastDismissalReason_ApplicationHidden:
            tLog << L"The application hid the toast using ToastNotifier.hide()";
            m_userAction = NtfyToastActions::Actions::Hidden;
            break;

        case ToastDismissalReason_UserCanceled:
            tLog << L"The user dismissed this toast";
            m_userAction = NtfyToastActions::Actions::Dismissed;
            break;

        case ToastDismissalReason_TimedOut:
            tLog << L"The toast has timed out";
            m_userAction = NtfyToastActions::Actions::Timedout;
            break;
        }
    }

    if (!m_toast.pipeName().empty()) {
        Utils::writePipe(m_toast.pipeName(), m_toast.formatAction(m_userAction));
    }

    SetEvent(m_event);

    return S_OK;
}

// DesktopToastFailedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification * /* sender */,
                                         _In_ IToastFailedEventArgs * /* e */)
{
    std::wcerr << L"NtfyToast encountered an error." << std::endl;
    std::wcerr << L"Please make sure that the app id is set correctly." << std::endl;
    std::wcerr << L"Command Line: " << GetCommandLineW() << std::endl;
    m_userAction = NtfyToastActions::Actions::Error;

    SetEvent(m_event);

    return S_OK;
}
