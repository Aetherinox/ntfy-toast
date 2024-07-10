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

#include "linkhelper.h"
#include "toasteventhandler.h"
#include "utils.h"

#include <propvarutil.h>
#include <comdef.h>

#include <iostream>
#include <sstream>

// compat with older sdk
#ifndef INIT_PKEY_AppUserModel_ToastActivatorCLSID
EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY PKEY_AppUserModel_ToastActivatorCLSID = {
    { 0x9F4C2855, 0x9F79, 0x4B39, { 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 } }, 26
};

#define INIT_PKEY_AppUserModel_ToastActivatorCLSID                                                 \
    {                                                                                              \
        { 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 }, 26         \
    }
#endif //#ifndef INIT_PKEY_AppUserModel_ToastActivatorCLSID

HRESULT LinkHelper::tryCreateShortcut(const std::filesystem::path &shortcutPath,
                                      const std::filesystem::path &exePath,
                                      const std::wstring &appID, const std::wstring &callbackUUID)
{
    std::filesystem::path path = shortcutPath;
    if (path.is_relative()) {
        path = startmenuPath() / path;
    }
    // make sure the extension is set
    path.replace_extension(L".lnk");

    if (std::filesystem::exists(path)) {
        tLog << L"Path: " << path << L" already exists, skip creation of shortcut";
        return S_OK;
    }
    if (!std::filesystem::exists(path.parent_path())
        && !std::filesystem::create_directories(path.parent_path())) {
        tLog << L"Failed to create dir: " << path.parent_path();
        return S_FALSE;
    }
    return installShortcut(path, exePath, appID, callbackUUID);
}

HRESULT LinkHelper::tryCreateShortcut(const std::filesystem::path &shortcutPath,
                                      const std::wstring &appID, const std::wstring &callbackUUID)
{
    return tryCreateShortcut(shortcutPath, Utils::selfLocate(), appID, callbackUUID);
}

// Install the shortcut
HRESULT LinkHelper::installShortcut(const std::filesystem::path &shortcutPath,
                                    const std::filesystem::path &exePath, const std::wstring &appID,
                                    const std::wstring &callbackUUID)
{
    std::wcout << L"Installing shortcut: " << shortcutPath << L" " << exePath << L" " << appID
               << std::endl;
    tLog << L"Installing shortcut: " << shortcutPath << L" " << exePath << L" " << appID << L" "
         << callbackUUID;
    if (!callbackUUID.empty()) {
        /**
         * Add CToastNotificationActivationCallback to registry
         * Required to use the CToastNotificationActivationCallback for buttons and textbox
         * interactions. windows.ui.notifications does not support user interaction from cpp
         */
        const std::wstring locPath = Utils::selfLocate().wstring();
        const std::wstring url = [&callbackUUID] {
            std::wstringstream url;
            url << L"SOFTWARE\\Classes\\CLSID\\" << callbackUUID << L"\\LocalServer32";
            return url.str();
        }();
        tLog << url;
        ST_RETURN_ON_ERROR(HRESULT_FROM_WIN32(
                ::RegSetKeyValueW(HKEY_CURRENT_USER, url.c_str(), nullptr, REG_SZ, locPath.c_str(),
                                  static_cast<DWORD>(locPath.size() * sizeof(wchar_t)))));
    }

    ComPtr<IShellLink> shellLink;
    ST_RETURN_ON_ERROR(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_PPV_ARGS(&shellLink)));
    ST_RETURN_ON_ERROR(shellLink->SetPath(exePath.c_str()));
    ST_RETURN_ON_ERROR(shellLink->SetArguments(L""));

    ComPtr<IPropertyStore> propertyStore;
    ST_RETURN_ON_ERROR(shellLink.As(&propertyStore));

    PROPVARIANT appIdPropVar;
    ST_RETURN_ON_ERROR(InitPropVariantFromString(appID.c_str(), &appIdPropVar));
    ST_RETURN_ON_ERROR(propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar));
    PropVariantClear(&appIdPropVar);

    if (!callbackUUID.empty()) {
        GUID guid;
        ST_RETURN_ON_ERROR(CLSIDFromString(callbackUUID.c_str(), &guid));

        tLog << guid.Data1;
        PROPVARIANT toastActivatorPropVar = {};
        toastActivatorPropVar.vt = VT_CLSID;
        toastActivatorPropVar.puuid = &guid;
        ST_RETURN_ON_ERROR(propertyStore->SetValue(PKEY_AppUserModel_ToastActivatorCLSID,
                                                   toastActivatorPropVar));
    }
    ST_RETURN_ON_ERROR(propertyStore->Commit());

    ComPtr<IPersistFile> persistFile;
    ST_RETURN_ON_ERROR(shellLink.As(&persistFile));
    return persistFile->Save(shortcutPath.c_str(), true);
}

std::filesystem::path LinkHelper::startmenuPath()
{
    wchar_t buffer[MAX_PATH];
    std::wstringstream path;

    if (GetEnvironmentVariable(L"APPDATA", buffer, MAX_PATH) > 0) {
        path << buffer << L"\\Microsoft\\Windows\\Start Menu\\Programs\\";
    }

    return path.str();
}
