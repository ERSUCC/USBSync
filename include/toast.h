#pragma once

#include <string>
#include <wchar.h>

#include <roapi.h>
#include <windows.ui.notifications.h>
#include <wrl.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

namespace WRL = Microsoft::WRL;
namespace Wrappers = WRL::Wrappers;
namespace Notifications = ABI::Windows::UI::Notifications;
namespace Dom = ABI::Windows::Data::Xml::Dom;

struct USBSyncToast
{
    static void initToast();
    static void deinitToast();
    static void displayToast(const std::string& message);
};
