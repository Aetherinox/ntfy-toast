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

#include <string>
#include <map>
#include <vector>

class NtfyToastActions
{
public:
    enum class Actions {
        Clicked,
        Hidden,
        Dismissed,
        Timedout,
        ButtonClicked,
        TextEntered,

        Error = -1
    };

    static const inline std::wstring &getActionString(const Actions &a)
    {
        return actionMap().at(a);
    }

    template<typename T>
    static inline NtfyToastActions::Actions getAction(const T &s)
    {
        for (const auto &a : actionMap()) {
            if (a.second.compare(s) == 0) {
                return a.first;
            }
        }
        return NtfyToastActions::Actions::Error;
    }

private:
    static const std::map<Actions, std::wstring> &actionMap()
    {
        static const std::map<Actions, std::wstring> _ActionStrings = {
            { Actions::Clicked, L"clicked" },
            { Actions::Hidden, L"hidden" },
            { Actions::Dismissed, L"dismissed" },
            { Actions::Timedout, L"timedout" },
            { Actions::ButtonClicked, L"buttonClicked" },
            { Actions::TextEntered, L"textEntered" }
        };
        return _ActionStrings;
    }
};
