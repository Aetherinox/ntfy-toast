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
#include "linkhelper.h"
#include "utils.h"
#include "config.h"

#include <wrl\wrappers\corewrappers.h>
#include <sstream>
#include <iostream>

using namespace Microsoft::WRL;
using namespace ABI::Windows::UI;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace Windows::Foundation;
using namespace Wrappers;

namespace {
constexpr DWORD EVENT_TIMEOUT = 60 * 1000; // one minute should be more than enough
}

class NtfyToastsPrivate
{
public:
    NtfyToastsPrivate(NtfyToasts *parent, const std::wstring &appID)
        : m_parent(parent), m_appID(appID), m_id(std::to_wstring(GetCurrentProcessId()))
    {

        ComPtr<IShellItem> app;
        if (FAILED(SHCreateItemFromParsingName(std::wstring(L"shell:AppsFolder\\" + m_appID).data(),
                                               nullptr, IID_PPV_ARGS(&app)))) {
            m_useFallbackMode = true;
            tLog << "AppUserModelId:" << m_appID
                 << " is not properly registered. Using fallback mode. Only click actions will be "
                    "availible";
        }

        HRESULT hr = GetActivationFactory(
                HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager)
                        .Get(),
                &m_toastManager);
        if (!SUCCEEDED(hr)) {
            std::wcerr << L"NtfyToasts: Failed to register com Factory, please make sure you "
                          L"correctly initialised with RO_INIT_MULTITHREADED"
                       << std::endl;
            m_action = NtfyToastActions::Actions::Error;
        }
    }
    NtfyToasts *m_parent;

    std::wstring m_appID;
    std::filesystem::path m_pipeName;
    std::filesystem::path m_application;

    std::wstring m_title;
    std::wstring m_body;
    std::filesystem::path m_image;
    std::wstring m_sound = L"Notification.Default";
    std::wstring m_id;
    std::wstring m_buttons;
    bool m_silent = false;
    bool m_textbox = false;
    bool m_persistent = false;

    bool m_useFallbackMode = false;

    Duration m_duration = Duration::Short;

    NtfyToastActions::Actions m_action = NtfyToastActions::Actions::Clicked;

    ComPtr<IXmlDocument> m_toastXml;
    ComPtr<IToastNotificationManagerStatics> m_toastManager;
    ComPtr<IToastNotifier> m_notifier;
    ComPtr<IToastNotification> m_notification;

    ComPtr<ToastEventHandler> m_eventHanlder;

    static HANDLE ctoastEvent()
    {
        static HANDLE _event = [] {
            std::wstringstream eventName;
            eventName << L"ToastActivationEvent" << GetCurrentProcessId();
            return CreateEvent(nullptr, true, false, eventName.str().c_str());
        }();
        return _event;
    }

    ComPtr<IToastNotificationHistory> getHistory()
    {
        ComPtr<IToastNotificationManagerStatics2> toastStatics2;
        if (ST_CHECK_RESULT(m_toastManager.As(&toastStatics2))) {
            ComPtr<IToastNotificationHistory> nativeHistory;
            ST_CHECK_RESULT(toastStatics2->get_History(&nativeHistory));
            return nativeHistory;
        }
        return {};
    }
};

NtfyToasts::NtfyToasts(const std::wstring &appID) : d(new NtfyToastsPrivate(this, appID))
{
    Utils::registerActivator();
}

NtfyToasts::~NtfyToasts()
{
    Utils::unregisterActivator();
    delete d;
}

HRESULT NtfyToasts::displayToast(const std::wstring &title, const std::wstring &body,
                                  const std::filesystem::path &image)
{
    // asume that we fail
    d->m_action = NtfyToastActions::Actions::Error;

    d->m_title = title;
    d->m_body = body;
    d->m_image = std::filesystem::absolute(image);

    /*
        Templates   : https://learn.microsoft.com/en-us/uwp/api/windows.ui.notifications.toasttemplatetype?view=winrt-26100#fields

        Structure:
            toast:      The launch attribute of this element defines what arguments will be passed back to your app when the user clicks your toast, allowing you to deep link into the correct content that the toast was displaying. To learn more, see Send a local app notification.
            visual:     This element represents visual portion of the toast, including the generic binding that contains text and images.
            actions:    This element represents interactive portion of the toast, including inputs and actions.
            audio:      This element specifies the audio played when the toast is shown to the user.
    */

    if (!d->m_image.empty()) {
        ST_RETURN_ON_ERROR(d->m_toastManager->GetTemplateContent(ToastTemplateType_ToastImageAndText02, &d->m_toastXml));
    } else {
        ST_RETURN_ON_ERROR(d->m_toastManager->GetTemplateContent(ToastTemplateType_ToastText02, &d->m_toastXml));
    }

    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeList> rootList;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->GetElementsByTagName(HStringReference(L"toast").Get(), &rootList));

    ComPtr<IXmlNode> root;
    ST_RETURN_ON_ERROR(rootList->Item(0, &root));
    ComPtr<IXmlNamedNodeMap> rootAttributes;
    ST_RETURN_ON_ERROR(root->get_Attributes(&rootAttributes));

    const auto data = formatAction(NtfyToastActions::Actions::Clicked);
    ST_RETURN_ON_ERROR(addAttribute(L"launch", rootAttributes.Get(), data));

    /*
        activationType 	Decides the type of activation that will be used when the user interacts with a specific action.

            "foreground"    - Default value. Your foreground app is launched.
            "background"    - Your corresponding background task is triggered, and you can execute code in the background without interrupting the user.
            "protocol"      - Launch a different app using protocol activation.
    */

    ST_RETURN_ON_ERROR(addAttribute(L"activationType", rootAttributes.Get(), L"foreground"));

    /*
        If -persistent is provided in arguments, notification will stay on screen until dismissed by user.

        scenario? = "reminder" | "alarm" | "incomingCall" | "urgent" 
    */

    if (d->m_persistent) {
        ST_RETURN_ON_ERROR(addAttribute(L"scenario", rootAttributes.Get(), L"incomingCall"));
    }

    ST_RETURN_ON_ERROR(addAttribute(L"duration", rootAttributes.Get(),
                                    d->m_duration == Duration::Short ? L"short" : L"long"));
    // Adding buttons
    if (!d->m_buttons.empty()) {
        setButtons(root);
    } else if (d->m_textbox) {
        setTextBox(root);
    }
    
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> audioElement;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->CreateElement(HStringReference(L"audio").Get(), &audioElement));

    ComPtr<IXmlNode> audioNodeTmp;
    ST_RETURN_ON_ERROR(audioElement.As(&audioNodeTmp));

    ComPtr<IXmlNode> audioNode;
    ST_RETURN_ON_ERROR(root->AppendChild(audioNodeTmp.Get(), &audioNode));

    ComPtr<IXmlNamedNodeMap> attributes;
    ST_RETURN_ON_ERROR(audioNode->get_Attributes(&attributes));
    ST_RETURN_ON_ERROR(addAttribute(L"src", attributes.Get()));
    ST_RETURN_ON_ERROR(addAttribute(L"silent", attributes.Get()));

    //    printXML();

    if (!d->m_image.empty()) {
        ST_RETURN_ON_ERROR(setImage());
    }

    ST_RETURN_ON_ERROR(setSound());

    ST_RETURN_ON_ERROR(setTextValues());

    std::wcerr  << L" --- " << d->m_toastXml << std::endl;

    printXML();
    ST_RETURN_ON_ERROR(createToast());
    d->m_action = NtfyToastActions::Actions::Clicked;
    return S_OK;
}

NtfyToastActions::Actions NtfyToasts::userAction()
{
    if (d->m_eventHanlder.Get()) {
        HANDLE event = d->m_eventHanlder.Get()->event();
        if (WaitForSingleObject(event, EVENT_TIMEOUT) == WAIT_TIMEOUT) {
            d->m_action = NtfyToastActions::Actions::Error;
        } else {
            d->m_action = d->m_eventHanlder.Get()->userAction();
        }
        // the initial value is NtfyToastActions::Actions::Hidden so if no action happend when we
        // end up here, a hide was requested
        if (d->m_action == NtfyToastActions::Actions::Hidden) {
            d->m_notifier->Hide(d->m_notification.Get());
            tLog << L"The application hid the toast using ToastNotifier.hide()";
        }
        CloseHandle(event);
    }
    return d->m_action;
}

bool NtfyToasts::closeNotification()
{
    std::wstringstream eventName;
    eventName << L"ToastEvent" << d->m_id;

    HANDLE event = OpenEventW(EVENT_ALL_ACCESS, FALSE, eventName.str().c_str());
    if (event) {
        SetEvent(event);
        return true;
    }
    if (auto history = d->getHistory()) {
        if (ST_CHECK_RESULT(history->RemoveGroupedTagWithId(
                    HStringReference(d->m_id.c_str()).Get(), HStringReference(L"NtfyToast").Get(),
                    HStringReference(d->m_appID.c_str()).Get()))) {
            return true;
        }
    }
    tLog << "Notification " << d->m_id << " does not exist";
    return false;
}

void NtfyToasts::setSound(const std::wstring &soundFile)
{
    d->m_sound = soundFile;
}

void NtfyToasts::setSilent(bool silent)
{
    d->m_silent = silent;
}

void NtfyToasts::setPersistent(bool persistent)
{
    d->m_persistent = persistent;
}

void NtfyToasts::setId(const std::wstring &id)
{
    if (!id.empty()) {
        d->m_id = id;
    }
}

std::wstring NtfyToasts::id() const
{
    return d->m_id;
}

void NtfyToasts::setButtons(const std::wstring &buttons)
{
    d->m_buttons = buttons;
}

void NtfyToasts::setTextBoxEnabled(bool textBoxEnabled)
{
    d->m_textbox = textBoxEnabled;
}

// Set the value of the "src" attribute of the "image" node
HRESULT NtfyToasts::setImage()
{
    ComPtr<IXmlNodeList> nodeList;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->GetElementsByTagName(HStringReference(L"image").Get(), &nodeList));

    ComPtr<IXmlNode> imageNode;
    ST_RETURN_ON_ERROR(nodeList->Item(0, &imageNode));

    ComPtr<IXmlNamedNodeMap> attributes;
    ST_RETURN_ON_ERROR(imageNode->get_Attributes(&attributes));

    ComPtr<IXmlNode> srcAttribute;

    /*
    ST_RETURN_ON_ERROR(attributes->GetNamedItem(HStringReference(L"placement").Get(), &srcAttribute));
    setNodeValueString(HStringReference(L"hero").Get(), srcAttribute.Get());
    */

    ST_RETURN_ON_ERROR(attributes->GetNamedItem(HStringReference(L"src").Get(), &srcAttribute));
    return setNodeValueString(HStringReference(d->m_image.wstring().c_str()).Get(), srcAttribute.Get());
}

HRESULT NtfyToasts::setSound()
{
    ComPtr<IXmlNodeList> nodeList;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->GetElementsByTagName(HStringReference(L"audio").Get(), &nodeList));

    ComPtr<IXmlNode> audioNode;
    ST_RETURN_ON_ERROR(nodeList->Item(0, &audioNode));

    ComPtr<IXmlNamedNodeMap> attributes;

    ST_RETURN_ON_ERROR(audioNode->get_Attributes(&attributes));
    ComPtr<IXmlNode> srcAttribute;

    ST_RETURN_ON_ERROR(attributes->GetNamedItem(HStringReference(L"src").Get(), &srcAttribute));
    std::wstring sound;
    if (d->m_sound.find(L"ms-winsoundevent:") == std::wstring::npos) {
        sound = L"ms-winsoundevent:";
        sound.append(d->m_sound);
    } else {
        sound = d->m_sound;
    }

    ST_RETURN_ON_ERROR(
            setNodeValueString(HStringReference(sound.c_str()).Get(), srcAttribute.Get()));

    ST_RETURN_ON_ERROR(attributes->GetNamedItem(HStringReference(L"silent").Get(), &srcAttribute));

    return setNodeValueString(HStringReference(d->m_silent ? L"true" : L"false").Get(),
                              srcAttribute.Get());
}

// Set the values of each of the text nodes
HRESULT NtfyToasts::setTextValues()
{
    ComPtr<IXmlNodeList> nodeList;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->GetElementsByTagName(HStringReference(L"text").Get(), &nodeList));
    
    // create the title
    ComPtr<IXmlNode> textNode;
    ST_RETURN_ON_ERROR(nodeList->Item(0, &textNode));
    ST_RETURN_ON_ERROR(
            setNodeValueString(HStringReference(d->m_title.c_str()).Get(), textNode.Get()));

    ST_RETURN_ON_ERROR(nodeList->Item(1, &textNode));
    return setNodeValueString(HStringReference(d->m_body.c_str()).Get(), textNode.Get());
}

HRESULT NtfyToasts::setButtons(ComPtr<IXmlNode> root)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> actionsElement;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->CreateElement(HStringReference(L"actions").Get(), &actionsElement));

    ComPtr<IXmlNode> actionsNodeTmp;
    ST_RETURN_ON_ERROR(actionsElement.As(&actionsNodeTmp));

    ComPtr<IXmlNode> actionsNode;
    ST_RETURN_ON_ERROR(root->AppendChild(actionsNodeTmp.Get(), &actionsNode));

    std::wstring buttonText;
    std::wstringstream wss(d->m_buttons);
    while (std::getline(wss, buttonText, L';')) {
        ST_RETURN_ON_ERROR(createNewActionButton(actionsNode, buttonText));
    }
    return S_OK;
}

HRESULT NtfyToasts::setTextBox(ComPtr<IXmlNode> root)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> actionsElement;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->CreateElement(HStringReference(L"actions").Get(), &actionsElement));

    ComPtr<IXmlNode> actionsNodeTmp;
    ST_RETURN_ON_ERROR(actionsElement.As(&actionsNodeTmp));

    ComPtr<IXmlNode> actionsNode;
    ST_RETURN_ON_ERROR(root->AppendChild(actionsNodeTmp.Get(), &actionsNode));

    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> inputElement;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->CreateElement(HStringReference(L"input").Get(), &inputElement));

    ComPtr<IXmlNode> inputNodeTmp;
    ST_RETURN_ON_ERROR(inputElement.As(&inputNodeTmp));

    ComPtr<IXmlNode> inputNode;
    ST_RETURN_ON_ERROR(actionsNode->AppendChild(inputNodeTmp.Get(), &inputNode));

    ComPtr<IXmlNamedNodeMap> inputAttributes;
    ST_RETURN_ON_ERROR(inputNode->get_Attributes(&inputAttributes));

    ST_RETURN_ON_ERROR(addAttribute(L"id", inputAttributes.Get(), L"textBox"));
    ST_RETURN_ON_ERROR(addAttribute(L"type", inputAttributes.Get(), L"text"));
    ST_RETURN_ON_ERROR(addAttribute(L"placeHolderContent", inputAttributes.Get(), L"Type a reply"));

    ComPtr<IXmlElement> actionElement;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->CreateElement(HStringReference(L"action").Get(), &actionElement));

    ComPtr<IXmlNode> actionNodeTmp;
    ST_RETURN_ON_ERROR(actionElement.As(&actionNodeTmp));

    ComPtr<IXmlNode> actionNode;
    ST_RETURN_ON_ERROR(actionsNode->AppendChild(actionNodeTmp.Get(), &actionNode));

    ComPtr<IXmlNamedNodeMap> actionAttributes;
    ST_RETURN_ON_ERROR(actionNode->get_Attributes(&actionAttributes));

    ST_RETURN_ON_ERROR(addAttribute(L"content", actionAttributes.Get(), L"Send"));

    const auto data = formatAction(NtfyToastActions::Actions::TextEntered);

    ST_RETURN_ON_ERROR(addAttribute(L"arguments", actionAttributes.Get(), data));
    return addAttribute(L"hint-inputId", actionAttributes.Get(), L"textBox");
}

HRESULT NtfyToasts::setEventHandler(ComPtr<IToastNotification> toast)
{
    // Register the event handlers
    EventRegistrationToken activatedToken, dismissedToken, failedToken;
    ComPtr<ToastEventHandler> eventHandler(new ToastEventHandler(*this));

    ST_RETURN_ON_ERROR(toast->add_Activated(eventHandler.Get(), &activatedToken));
    ST_RETURN_ON_ERROR(toast->add_Dismissed(eventHandler.Get(), &dismissedToken));
    ST_RETURN_ON_ERROR(toast->add_Failed(eventHandler.Get(), &failedToken));
    d->m_eventHanlder = eventHandler;
    return S_OK;
}

HRESULT NtfyToasts::setNodeValueString(const HSTRING &inputString, IXmlNode *node)
{
    ComPtr<IXmlText> inputText;
    ST_RETURN_ON_ERROR(d->m_toastXml->CreateTextNode(inputString, &inputText));

    ComPtr<IXmlNode> inputTextNode;
    ST_RETURN_ON_ERROR(inputText.As(&inputTextNode));

    ComPtr<IXmlNode> pAppendedChild;
    return node->AppendChild(inputTextNode.Get(), &pAppendedChild);
}

HRESULT NtfyToasts::addAttribute(const std::wstring &name, IXmlNamedNodeMap *attributeMap)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlAttribute> srcAttribute;
    HRESULT hr =
            d->m_toastXml->CreateAttribute(HStringReference(name.c_str()).Get(), &srcAttribute);

    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> node;
        hr = srcAttribute.As(&node);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> pNode;
            hr = attributeMap->SetNamedItem(node.Get(), &pNode);
        }
    }
    return hr;
}

HRESULT NtfyToasts::addAttribute(const std::wstring &name, IXmlNamedNodeMap *attributeMap,
                                  const std::wstring &value)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlAttribute> srcAttribute;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->CreateAttribute(HStringReference(name.c_str()).Get(), &srcAttribute));

    ComPtr<IXmlNode> node;
    ST_RETURN_ON_ERROR(srcAttribute.As(&node));

    ComPtr<IXmlNode> pNode;
    ST_RETURN_ON_ERROR(attributeMap->SetNamedItem(node.Get(), &pNode));

    return setNodeValueString(HStringReference(value.c_str()).Get(), node.Get());
}

HRESULT NtfyToasts::createNewActionButton(ComPtr<IXmlNode> actionsNode, const std::wstring &value)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> actionElement;
    ST_RETURN_ON_ERROR(
            d->m_toastXml->CreateElement(HStringReference(L"action").Get(), &actionElement));

    ComPtr<IXmlNode> actionNodeTmp;
    ST_RETURN_ON_ERROR(actionElement.As(&actionNodeTmp));

    ComPtr<IXmlNode> actionNode;
    ST_RETURN_ON_ERROR(actionsNode->AppendChild(actionNodeTmp.Get(), &actionNode));

    ComPtr<IXmlNamedNodeMap> actionAttributes;
    ST_RETURN_ON_ERROR(actionNode->get_Attributes(&actionAttributes));

    ST_RETURN_ON_ERROR(addAttribute(L"content", actionAttributes.Get(), value));

    const auto data =
            formatAction(NtfyToastActions::Actions::ButtonClicked, { { L"button", value } });
    ST_RETURN_ON_ERROR(addAttribute(L"arguments", actionAttributes.Get(), data));

    return addAttribute(L"activationType", actionAttributes.Get(), L"foreground");
}

void NtfyToasts::printXML()
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeSerializer> s;
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument> ss(d->m_toastXml);
    ss.As(&s);
    HSTRING string;
    s->GetXml(&string);

    PCWSTR str = WindowsGetStringRawBuffer(string, nullptr);

    /*
        debug > print xml
        std::wcerr << str << std::endl;
    */

    tLog << L"------------------------\n\t\t\t" << str << L"\n\t\t" << L"------------------------";
}

std::filesystem::path NtfyToasts::pipeName() const
{
    return d->m_pipeName;
}

void NtfyToasts::setPipeName(const std::filesystem::path &pipeName)
{
    d->m_pipeName = pipeName;
}

std::filesystem::path NtfyToasts::application() const
{
    return d->m_application;
}

void NtfyToasts::setApplication(const std::filesystem::path &application)
{
    d->m_application = application;
}

void NtfyToasts::setDuration(Duration duration)
{
    d->m_duration = duration;
}

Duration NtfyToasts::duration() const
{
    return d->m_duration;
}

std::wstring NtfyToasts::formatAction(
        const NtfyToastActions::Actions &action,
        const std::vector<std::pair<std::wstring_view, std::wstring_view>> &extraData) const
{
    const auto pipe = d->m_pipeName.wstring();
    const auto application = d->m_application.wstring();
    std::vector<std::pair<std::wstring_view, std::wstring_view>> data = {
        { L"action", NtfyToastActions::getActionString(action) },
        { L"notificationId", std::wstring_view(d->m_id) },
        { L"pipe", std::wstring_view(pipe) },
        { L"application", std::wstring_view(application) }
    };
    data.insert(data.end(), extraData.cbegin(), extraData.cend());
    return Utils::formatData(data);
}

// Create and display the toast
HRESULT NtfyToasts::createToast()
{
    ST_RETURN_ON_ERROR(d->m_toastManager->CreateToastNotifierWithId(
            HStringReference(d->m_appID.c_str()).Get(), &d->m_notifier));

    ComPtr<IToastNotificationFactory> factory;
    ST_RETURN_ON_ERROR(GetActivationFactory(
            HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
            &factory));

    ST_RETURN_ON_ERROR(factory->CreateToastNotification(d->m_toastXml.Get(), &d->m_notification));

    ComPtr<Notifications::IToastNotification2> toastV2;

    if (SUCCEEDED(d->m_notification.As(&toastV2))) {
        ST_RETURN_ON_ERROR(toastV2->put_Tag(HStringReference(d->m_id.c_str()).Get()));
        ST_RETURN_ON_ERROR(toastV2->put_Group(HStringReference(L"NtfyToast").Get()));
    }

    std::wstring error;
    NotificationSetting setting = NotificationSetting_Enabled;

    if (!ST_CHECK_RESULT(d->m_notifier->get_Setting(&setting))) {
        tLog << "Failed to retreive NotificationSettings ensure your appId is registered";
    }

    switch (setting) {
    case NotificationSetting_Enabled:
        ST_RETURN_ON_ERROR(setEventHandler(d->m_notification));
        break;

    case NotificationSetting_DisabledForApplication:
        error = L"DisabledForApplication";
        break;

    case NotificationSetting_DisabledForUser:
        error = L"DisabledForUser";
        break;

    case NotificationSetting_DisabledByGroupPolicy:
        error = L"DisabledByGroupPolicy";
        break;

    case NotificationSetting_DisabledByManifest:
        error = L"DisabledByManifest";
        break;

    }
    if (!error.empty()) {
        std::wstringstream err;
        err << L"Notifications are disabled\n"
            << L"Reason: " << error << L" Please make sure that the app id is set correctly.\n"
            << L"Command Line: " << GetCommandLineW();
        tLog << err.str();
        std::wcerr << err.str() << std::endl;
    }
    return d->m_notifier->Show(d->m_notification.Get());
}

std::wstring NtfyToasts::version()
{
    return NTFYTOAST_VERSION;
}

HRESULT NtfyToasts::backgroundCallback(const std::wstring &appUserModelId,
                                        const std::wstring &invokedArgs, const std::wstring &msg)
{
    tLog << "CToastNotificationActivationCallback::Activate: " << appUserModelId << " : "
         << invokedArgs << " : " << msg;
    const auto dataMap = Utils::splitData(invokedArgs);
    const auto action = NtfyToastActions::getAction(dataMap.at(L"action"));
    std::wstring dataString;
    if (action == NtfyToastActions::Actions::TextEntered) {
        std::wstringstream sMsg;
        sMsg << invokedArgs << L"text=" << msg;
        dataString = sMsg.str();
    } else {
        dataString = invokedArgs;
    }
    const auto pipe = dataMap.find(L"pipe");
    if (pipe != dataMap.cend()) {
        if (!Utils::writePipe(pipe->second, dataString)) {
            const auto app = dataMap.find(L"application");
            if (app != dataMap.cend()) {
                if (Utils::startProcess(app->second)) {
                    Utils::writePipe(pipe->second, dataString, true);
                }
            }
        }
    }

    tLog << dataString;
    if (!SetEvent(NtfyToastsPrivate::ctoastEvent())) {
        tLog << "SetEvent failed" << GetLastError();
        return S_FALSE;
    }
    return S_OK;
}
void NtfyToasts::waitForCallbackActivation()
{
    Utils::registerActivator();
    WaitForSingleObject(NtfyToastsPrivate::ctoastEvent(), EVENT_TIMEOUT);
    Utils::unregisterActivator();
}

bool NtfyToasts::useFalbackMode() const
{
    return d->m_useFallbackMode;
}