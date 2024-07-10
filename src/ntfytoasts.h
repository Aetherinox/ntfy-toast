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

#include "ntfytoastactions.h"
#include "libntfytoast_export.h"

#include <sdkddkver.h>

// Windows Header Files:
#include <windows.h>
#include <sal.h>
#include <psapi.h>
#include <strsafe.h>
#include <objbase.h>
#include <shobjidl.h>
#include <functiondiscoverykeys.h>
#include <guiddef.h>
#include <shlguid.h>

#include <wrl/client.h>
#include <wrl/implements.h>
#include <windows.ui.notifications.h>

#include <filesystem>
#include <string>
#include <vector>

using namespace Microsoft::WRL;
using namespace ABI::Windows::Data::Xml::Dom;

/*
    Windows API only allows for two durations to be specified
        - Short     7 seconds
        - Long      25 seconds

    To specify a notification that will stay up on the user's screen until
    they interact with it; you must specify the notification as an alarm.
*/

enum class Duration {
    Short,
    Long
};

class LIBNTFYTOAST_EXPORT NtfyToasts
{
public:
    static std::wstring version();
    static void waitForCallbackActivation();
    static HRESULT backgroundCallback(const std::wstring &appUserModelId,
                                      const std::wstring &invokedArgs, const std::wstring &msg);

    NtfyToasts(const std::wstring &appID);
    ~NtfyToasts();

    HRESULT displayToast(const std::wstring &title, const std::wstring &body,
                         const std::filesystem::path &image);

    NtfyToastActions::Actions userAction();
    bool closeNotification();

    void setSound(const std::wstring &soundFile);
    void setSilent(bool silent);
    void setPersistent(bool persistent);
    void setId(const std::wstring &id);
    std::wstring id() const;

    void setButtons(const std::wstring &buttons);
    void setTextBoxEnabled(bool textBoxEnabled);

    std::filesystem::path pipeName() const;
    void setPipeName(const std::filesystem::path &pipeName);

    std::filesystem::path application() const;
    void setApplication(const std::filesystem::path &application);

    Duration duration() const;
    void setDuration(Duration duration);

    std::wstring formatAction(const NtfyToastActions::Actions &action,
                              const std::vector<std::pair<std::wstring_view, std::wstring_view>>
                                      &extraData = {}) const;

    /**
     * Returns true if the appID is not properly registered
     * This usually means that no shortcut with the appID is installed.
     * In fallback mode no text replies or buttons are available.
     */

    bool useFalbackMode() const;

private:
    HRESULT createToast();
    HRESULT setImage();
    HRESULT setSound();
    HRESULT setTextValues();
    HRESULT setButtons(ComPtr<IXmlNode> root);
    HRESULT setTextBox(ComPtr<IXmlNode> root);
    HRESULT setEventHandler(
            Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> toast);
    HRESULT setNodeValueString(const HSTRING &onputString,
                               ABI::Windows::Data::Xml::Dom::IXmlNode *node);
    HRESULT addAttribute(const std::wstring &name,
                         ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap *attributeMap);
    HRESULT addAttribute(const std::wstring &name,
                         ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap *attributeMap,
                         const std::wstring &value);
    HRESULT createNewActionButton(ComPtr<IXmlNode> actionsNode, const std::wstring &value);

    void printXML();

    friend class NtfyToastsPrivate;
    NtfyToastsPrivate *d;
};
