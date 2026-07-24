#include "toast.h"

#define CHECK(expr) if (expr != S_OK) return

void USBSyncToast::initToast()
{
    if (RoInitialize(RO_INIT_MULTITHREADED) != S_OK)
    {
        return;
    }

    WRL::Module<WRL::OutOfProc>::Create([]{});
    WRL::Module<WRL::OutOfProc>::GetModule().IncrementObjectCount();

    if (WRL::Module<WRL::OutOfProc>::GetModule().RegisterObjects() != S_OK)
    {
        RoUninitialize();

        return;
    }
}

void USBSyncToast::deinitToast()
{
    RoUninitialize();
}

void USBSyncToast::displayToast(const std::string& message)
{
    WRL::ComPtr<Notifications::IToastNotificationManagerStatics> manager;

    CHECK(Windows::Foundation::GetActivationFactory(Wrappers::HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), &manager));

    wchar_t appId[MAX_PATH];

    mbstowcs(appId, USBSYNC_APP_ID, MAX_PATH);

    WRL::ComPtr<Notifications::IToastNotifier> notifier;

    CHECK(manager->CreateToastNotifierWithId(Wrappers::HStringReference(appId).Get(), &notifier));

    WRL::ComPtr<Notifications::IToastNotificationFactory> factory;

    CHECK(Windows::Foundation::GetActivationFactory(Wrappers::HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), &factory));

    WRL::ComPtr<Dom::IXmlDocument> document;

    CHECK(manager->GetTemplateContent(Notifications::ToastTemplateType_ToastText01, &document));

    WRL::ComPtr<Dom::IXmlNodeList> elements;

    CHECK(document->GetElementsByTagName(Wrappers::HStringReference(L"text").Get(), &elements));

    UINT32 length;

    CHECK(elements->get_Length(&length));

    if (length != 1)
    {
        return;
    }

    WRL::ComPtr<Dom::IXmlNode> node;

    CHECK(elements->Item(0, &node));

    wchar_t wmessage[1024];

    mbstowcs(wmessage, message.c_str(), 1024);

    WRL::ComPtr<Dom::IXmlText> text;

    CHECK(document->CreateTextNode(Wrappers::HStringReference(wmessage).Get(), &text));

    WRL::ComPtr<Dom::IXmlNode> textNode;

    CHECK(text.As(&textNode));

    WRL::ComPtr<Dom::IXmlNode> child;

    CHECK(node->AppendChild(textNode.Get(), &child));

    WRL::ComPtr<Notifications::IToastNotification> notification;

    CHECK(factory->CreateToastNotification(document.Get(), &notification));
    CHECK(notifier->Show(notification.Get()));
}
