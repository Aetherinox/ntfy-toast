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
#include "config.h"

#include "toasteventhandler.h"

#include "ntfytoastactioncenterintegration.h"

#include "linkhelper.h"
#include "utils.h"

#include <cmrc/cmrc.hpp>

#include <appmodel.h>
#include <shellapi.h>
#include <roapi.h>

#include <algorithm>
#include <functional>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

CMRC_DECLARE(NtfyToastResource);

std::wstring getAppId(const std::wstring &pid, const std::wstring &fallbackAppID)
{
    if (pid.empty()) {
        return fallbackAppID;
    }
    const int _pid = std::stoi(pid);
    const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, _pid);
    if (!process) {
        tLog << "Failed to retreive appid for " << _pid
             << " Failed to retrive process hanlde: " << Utils::formatWinError(GetLastError());
        return fallbackAppID;
    }
    uint32_t size = 0;
    long rc = GetApplicationUserModelId(process, &size, nullptr);
    if (rc != ERROR_INSUFFICIENT_BUFFER) {
        if (rc == APPMODEL_ERROR_NO_APPLICATION) {
            tLog << "Failed to retreive appid for " << _pid << " Process is a desktop application";
        } else {
            tLog << "Failed to retreive appid for " << _pid
                 << " Error: " << Utils::formatWinError(rc);
        }
        CloseHandle(process);
        return fallbackAppID;
    }
    std::wstring out(size, 0);
    rc = GetApplicationUserModelId(process, &size, out.data());
    CloseHandle(process);
    if (rc != ERROR_SUCCESS) {
        tLog << "Failed to retreive appid for " << _pid << " Error: " << Utils::formatWinError(rc);
        return fallbackAppID;
    }
    // strip 0
    out.resize(out.size() - 1);
    tLog << "AppId from pid" << out;
    return out;
}

void help(const std::wstring &error)
{
    if (!error.empty()) {
        std::wcerr << error << std::endl;
    } else {
        std::wcerr << L"Welcome to NtfyToast " << NtfyToasts::version() << "." << std::endl
                   << L"A command line application capable of creating Windows Toast notifications."
                   << std::endl;
    }
    const auto filesystem = cmrc::NtfyToastResource::get_filesystem();
    const auto help = filesystem.open("help.txt");
    std::wcerr << help.begin() << std::endl;
}

void version()
{
    std::wcerr  << std::endl
                << std::endl
                << L"---------------------------------------------------------------------" << std::endl
                << L" Version ................ v" << NtfyToasts::version() << std::endl
                << L" ToastActivatorCLSID .... " << NTFYTOAST_CALLBACK_GUID << std::endl
                << L"---------------------------------------------------------------------" << std::endl
                << std::endl
                << L" Copyright 2024-2024 Aetherinox" << std::endl
                << L" Copyright 2013-2019 Hannah von Reth <vonreth@kde.org>" << std::endl
                << std::endl
                << L" NtfyToast is free software: you can redistribute it and/or modify" << std::endl
                << L" it under the terms of the GNU Lesser General Public License as published by"
                << std::endl
                << L" the Free Software Foundation, either version 3 of the License, or" << std::endl
                << L" any later version." << std::endl;
}

std::filesystem::path getIcon()
{
    auto image = std::filesystem::temp_directory_path() / "ntfytoast" / NtfyToasts::version()
            / "data/logo.png";

    /*
        Paths
            - std::filesystem::temp_directory_path()
                C:\Users\$USER\AppData\Local\Temp\ntfytoast\0.9.0\logo.png

            - lnk
                C:\Users\$USER\AppData\Roaming\Microsoft\Windows\Start Menu\Programs
    */

    /*
        std::wcerr << L"" << std::filesystem::temp_directory_path() << "." << std::endl;
    */

    if (!std::filesystem::exists(image)) {
        std::filesystem::create_directories(image.parent_path());
        const auto filesystem = cmrc::NtfyToastResource::get_filesystem();
        const auto img = filesystem.open("256-256-ntfytoast.png");
        std::ofstream out(image, std::ios::binary);
        out.write(const_cast<char *>(img.begin()), img.size());
        out.close();
    }
    return image;
}

NtfyToastActions::Actions parse(std::vector<wchar_t *> args)
{
    HRESULT hr = S_OK;

    std::wstring appID;
    std::wstring pid;
    std::filesystem::path pipe;
    std::filesystem::path application;
    std::wstring title;
    std::wstring body;
    std::filesystem::path image;
    std::wstring id;
    std::wstring sound(L"Notification.Default");
    std::wstring buttons;
    Duration duration = Duration::Short;
    bool silent = false;
    bool persistent = false;
    bool closeNotify = false;
    bool isTextBoxEnabled = false;

    auto nextArg = [&](std::vector<wchar_t *>::const_iterator &it,
                       const std::wstring &helpText) -> std::wstring {
        if (it != args.cend()) {
            return *it++;
        } else {
            help(helpText);
            exit(static_cast<int>(NtfyToastActions::Actions::Error));
        }
    };

    auto it = args.begin() + 1;
    while (it != args.end()) {
        std::wstring arg(nextArg(it, L""));
        std::transform(arg.begin(), arg.end(), arg.begin(),
                       [](int i) -> int { return ::tolower(i); });

        /*
            Argument > Message
            Message displayed in notification

                -m <string message>
        */

        if (arg == L"-m") {
            body = nextArg(it,
                           L"Missing argument to -m.\n"
                           L"Supply argument as -m \"message string\"");

        /*
            Argument > Title
            Title / first line of text in notification

                -t <string title>
        */

        } else if (arg == L"-t") {
            title = nextArg(it,
                            L"Missing argument to -t.\n"
                            L"Supply argument as -t \"bold title string\"");

        /*
            Argument > Path (Image)
            Picture / image, local files only

                -p <image URI>
        */

        } else if (arg == L"-p") {
            image = nextArg(it, L"Missing argument to -p. Supply argument as -p \"image path\"");

        /*
            Argument > Sound
            Sound when notification opened

            Possible options:   http://msdn.microsoft.com/en-us/library/windows/apps/hh761492.aspx

                -s <sound URI>
        */

        } else if (arg == L"-s") {
            sound = nextArg(it,
                            L"Missing argument to -s.\n"
                            L"Supply argument as -s \"sound name\"");

        /*
            Argument > Duration
            How long a notification will appear on-screen before dismissing itself.

            If you want notifications to stay up indefinitely, see -persistent
                -d <string [short || long]>

            This argument only allows for two options
                - short     7 seconds
                - long      25 seconds
        */

        } else if (arg == L"-d") {
            std::wstring _d = nextArg(it,
                                      L"Missing argument to -d.\n"
                                      L"Supply argument as -d (short |long)");
            if (_d == L"short") {
                duration = Duration::Short;
            } else if (_d == L"long") {
                duration = Duration::Long;
            } else {
                help(_d + L" is not a valid");
                return NtfyToastActions::Actions::Error;
            }

        /*
            Argument > ID
            Sets id for a notification to be able to close it later

                -id <id>
        */

        } else if (arg == L"-id") {
            id = nextArg(it,
                         L"Missing argument to -id.\n"
                         L"Supply argument as -id \"id\"");

        /*
            Argument > Silent
            Disable playing sound when notification appears

                -silent
        */

        } else if (arg == L"-silent") {
            silent = true;

        /*
            Argument > Persistent
            Force notification to stay on screen

                -persistent
        */

        } else if (arg == L"-persistent") {
            persistent = true;

        /*
            Argument > App ID
            Don't create a shortcut but use the provided app id

                -appID <App.ID>
        */

        } else if (arg == L"-appid") {
            appID = nextArg(it,
                            L"Missing argument to -appID.\n"
                            L"Supply argument as -appID \"Your.APP.ID\"");

        /*
            Argument > Process ID
            Query the appid for the process <pid>, use -appID as fallback.
            (Only relevant for applications that might be packaged for the store

                -pid <pid>
        */

        } else if (arg == L"-pid") {
            pid = nextArg(it,
                          L"Missing argument to -pid.\n"
                          L"Supply argument as -pid \"pid\"");

        /*
            Argument > Pipe Name
            Name pipe which is used for callbacks

                -pipeName <\.\pipe\pipeName\>
        */

        } else if (arg == L"-pipename") {
            pipe = nextArg(it,
                           L"Missing argument to -pipeName.\n"
                           L"Supply argument as -pipeName \"\\.\\pipe\\foo\\\"");

        /*
            Argument > Application
            App to start if the pipe does not exist

                -application <C:\foo\bar.exe>
        */

        } else if (arg == L"-application") {
            application = nextArg(it,
                                  L"Missing argument to -application.\n"
                                  L"Supply argument as -application \"C:\\foo.exe\"");

        /*
            Argument > Buttons
            Buttons - List multiple buttons separated by `;`

                -b <btn1;bbtn2 string>
        */

        } else if (arg == L"-b") {
            buttons = nextArg(it,
                              L"Missing argument to -b.\n"
                              L"Supply argument for buttons as -b \"button1;button2\"");

        /*
            Argument > Textbox
            Textbox on the bottom line, only if buttons are not specified

                -tb
        */

        } else if (arg == L"-tb") {
            isTextBoxEnabled = true;

        /*
            Argument > Install
            Installs shortcut for application

                -install
        */

        } else if (arg == L"-install") {
            const std::wstring shortcut(
                    nextArg(it,
                            L"Missing argument to -install.\n"
                            L"Supply argument as -install \"path to your shortcut\" \"path to the "
                            L"application the shortcut should point to\" \"App.ID\""));

            const std::wstring exe(
                    nextArg(it,
                            L"Missing argument to -install.\n"
                            L"Supply argument as -install \"path to your shortcut\" \"path to the "
                            L"application the shortcut should point to\" \"App.ID\""));

            appID = nextArg(it,
                            L"Missing argument to -install.\n"
                            L"Supply argument as -install \"path to your shortcut\" \"path to the "
                            L"application the shortcut should point to\" \"App.ID\"");

            return SUCCEEDED(LinkHelper::tryCreateShortcut(
                           shortcut, exe, appID, NtfyToastActionCenterIntegration::uuid()))
                    ? NtfyToastActions::Actions::Clicked
                    : NtfyToastActions::Actions::Error;

        /*
            Argument > Close Notification
            Close an existing notification.

                -close <id>

            Assign an ID to a notification using:
                -id <id>
        */

        } else if (arg == L"-close") {
            id = nextArg(it,
                         L"Missing agument to -close"
                         L"Supply argument as -close \"id\"");
            closeNotify = true;

        /*
            Argument > Version
            Returns version information

                -v
        */

        } else if (arg == L"-v") {
            version();
            return NtfyToastActions::Actions::Clicked;

        /*
            Argument > Help
            Returns help menu

                -h
        */

        } else if (arg == L"-h") {
            help(L"");
            return NtfyToastActions::Actions::Clicked;

        /*
            Argument > Invalid
        */

        } else {
            std::wstringstream ws;
            ws << L"Unknown argument: " << arg << std::endl;
            help(ws.str());
            return NtfyToastActions::Actions::Error;
        }
    }

    appID = getAppId(pid, appID);

    if (appID.empty()) {
        std::wstringstream _appID;
        _appID << L"Ntfy.DesktopToasts." << NtfyToasts::version();
        appID = _appID.str();
        hr = LinkHelper::tryCreateShortcut(std::filesystem::path(L"NtfyToast")
                                                   / NtfyToasts::version() / L"NtfyToast",
                                           appID, NtfyToastActionCenterIntegration::uuid());

        if (!SUCCEEDED(hr)) {
            return NtfyToastActions::Actions::Error;
        }
    }

    if (closeNotify) {
        if (!id.empty()) {
            NtfyToasts app(appID);
            app.setId(id);
            if (app.closeNotification()) {
                return NtfyToastActions::Actions::Clicked;
            }
        } else {
            help(L"Close only works if an -id id was provided.");
        }
    } else {
        hr = (title.length() > 0 && body.length() > 0) ? S_OK : E_FAIL;
        if (SUCCEEDED(hr)) {
            if (isTextBoxEnabled) {
                if (pipe.empty()) {
                    std::wcerr << L"TextBox notifications only work if a pipe for the result "
                                  L"was provided"
                               << std::endl;
                    return NtfyToastActions::Actions::Error;
                }
            }

            /*
                Prepare notification parameters
            */

            if (image.empty()) {
                image = getIcon();
            }

            NtfyToasts app(appID);
            app.setPipeName(pipe);
            app.setApplication(application);
            app.setSilent(silent);
            app.setPersistent(persistent);
            app.setSound(sound);
            app.setId(id);
            app.setButtons(buttons);
            app.setTextBoxEnabled(isTextBoxEnabled);
            app.setDuration(duration);
            app.displayToast(title, body, image);
            return app.userAction();
        } else {
            help(L"");
            return NtfyToastActions::Actions::Clicked;
        }
    }

    return NtfyToastActions::Actions::Error;
}

NtfyToastActions::Actions handleEmbedded()
{
    NtfyToasts::waitForCallbackActivation();
    return NtfyToastActions::Actions::Clicked;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, wchar_t *, int)
{
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // atache to the parent console output, if its an interactive terminal
        CONSOLE_SCREEN_BUFFER_INFO csbi;

        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            FILE *dummy;
            _wfreopen_s(&dummy, L"CONOUT$", L"w", stdout);
            setvbuf(stdout, nullptr, _IONBF, 0);

            _wfreopen_s(&dummy, L"CONOUT$", L"w", stderr);
            setvbuf(stderr, nullptr, _IONBF, 0);
            std::ios::sync_with_stdio();
        }
    }

    const auto commandLine = GetCommandLineW();
    int argc;
    wchar_t **argv = CommandLineToArgvW(commandLine, &argc);

    tLog << commandLine;

    NtfyToastActions::Actions action = NtfyToastActions::Actions::Clicked;
    HRESULT hr = Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);

    if (SUCCEEDED(hr)) {
        if (std::wstring(commandLine).find(L"-Embedding") != std::wstring::npos) {
            action = handleEmbedded();
        } else {
            action = parse(std::vector<wchar_t *>(argv, argv + argc));
        }

        Windows::Foundation::Uninitialize();
    }

    return static_cast<int>(action);
}
