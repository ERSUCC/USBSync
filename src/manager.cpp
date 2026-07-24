#include "manager.h"

struct DECLSPEC_UUID(USBSYNC_GUID) USBSyncGuid;

constexpr UINT TRAY_MESSAGE = WM_APP + 1;

#define TRAY_CONTEXT_VIEW 0
#define TRAY_CONTEXT_QUIT 1

std::optional<GUID> getGUID(const std::wstring& path)
{
    HANDLE device = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);

    if (device == INVALID_HANDLE_VALUE)
    {
        return std::nullopt;
    }

    STORAGE_DEVICE_NUMBER_EX info;

    if (!DeviceIoControl(device, IOCTL_STORAGE_GET_DEVICE_NUMBER_EX, nullptr, 0, &info, sizeof(info), nullptr, nullptr))
    {
        CloseHandle(device);

        return std::nullopt;
    }

    CloseHandle(device);

    return info.DeviceGuid;
}

std::optional<std::string> getName(const std::wstring& path)
{
    HANDLE device = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);

    if (device == INVALID_HANDLE_VALUE)
    {
        return std::nullopt;
    }

    WCHAR name[MAX_PATH];

    if (!GetVolumeInformationByHandleW(device, name, MAX_PATH, nullptr, nullptr, nullptr, nullptr, 0))
    {
        CloseHandle(device);

        return std::nullopt;
    }

    CloseHandle(device);

    char cname[MAX_PATH];

    wcstombs(cname, name, MAX_PATH);

    return cname;
}

DWORD CALLBACK handleNotify(HCMNOTIFICATION handle, PVOID context, CM_NOTIFY_ACTION action, PCM_NOTIFY_EVENT_DATA data,
                            DWORD size)
{
    HandlerContext* handlerContext = (HandlerContext*)context;

    switch (action)
    {
        case CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL:
        {
            const std::optional<GUID> deviceGUID = getGUID(data->u.DeviceInterface.SymbolicLink);

            if (!deviceGUID)
            {
                return ERROR_SUCCESS;
            }

            WCHAR drives[MAX_PATH];

            DWORD length = GetLogicalDriveStringsW(MAX_PATH, drives);

            if (length == 0)
            {
                return ERROR_SUCCESS;
            }

            for (WCHAR* drive = drives; drive < drives + length - 1; drive += wcsnlen(drive, length) + 1)
            {
                if (const std::optional<GUID> guid = getGUID(L"\\\\.\\" + std::wstring(drive).substr(0, 2)))
                {
                    if (guid == deviceGUID)
                    {
                        char source[MAX_PATH];

                        wcstombs(source, drive, MAX_PATH);

                        const std::string name = getName(data->u.DeviceInterface.SymbolicLink).value_or("UNNAMED");

                        handlerContext->sync->beginSync(guid.value(), name, source);

                        return ERROR_SUCCESS;
                    }
                }
            }

            return ERROR_SUCCESS;
        }

        case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL:
            if (const std::optional<GUID> guid = getGUID(data->u.DeviceInterface.SymbolicLink))
            {
                handlerContext->sync->stopSync(guid.value());
            }

            return ERROR_SUCCESS;

        default:
            return ERROR_SUCCESS;
    }
}

void stopService()
{
    NOTIFYICONDATA data = { 0 };

    data.cbSize = sizeof(NOTIFYICONDATA);
    data.uFlags = NIF_GUID;
    data.guidItem = __uuidof(USBSyncGuid);

    Shell_NotifyIcon(NIM_DELETE, &data);

    PostQuitMessage(0);
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case TRAY_MESSAGE:
            switch (LOWORD(lParam))
            {
                case WM_CONTEXTMENU:
                    SetForegroundWindow(window);

                    HMENU menu = CreatePopupMenu();

                    AppendMenu(menu, 0, TRAY_CONTEXT_VIEW, "View Backups");
                    AppendMenu(menu, 0, TRAY_CONTEXT_QUIT, "Quit");

                    TrackPopupMenu(menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON, LOWORD(wParam),
                                   HIWORD(wParam), 0, window, nullptr);

                    break;
            }

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case TRAY_CONTEXT_VIEW:
                    ShellExecute(window, "explore", USBSync::backupRoot, nullptr, nullptr, SW_SHOW);

                    break;

                case TRAY_CONTEXT_QUIT:
                    stopService();

                    break;
            }

            break;

        case WM_DESTROY:
            stopService();

            break;
    }

    return DefWindowProc(window, message, wParam, lParam);
}

HandlerContext::HandlerContext() :
    sync(new USBSync()) {}

HandlerContext::~HandlerContext()
{
    delete sync;
}

void USBSyncManager::startService(HINSTANCE instance)
{
    HandlerContext* context = new HandlerContext();

    CM_NOTIFY_FILTER filter;

    filter.cbSize = sizeof(CM_NOTIFY_FILTER);
    filter.Flags = 0;
    filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    filter.Reserved = 0;
    filter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_VOLUME;

    if (CM_Register_Notification(&filter, context, handleNotify, &context->notifyContext) != CR_SUCCESS)
    {
        return;
    }

    WNDCLASSEX wndClass = { 0 };

    wndClass.cbSize = sizeof(wndClass);
    wndClass.hInstance = instance;
    wndClass.lpfnWndProc = windowProc;
    wndClass.lpszClassName = USBSYNC_APP_ID;

    if (!RegisterClassEx(&wndClass))
    {
        return;
    }

    HWND window = CreateWindow(USBSYNC_APP_ID, USBSYNC_APP_ID, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500,
                               500, nullptr, nullptr, instance, nullptr);

    if (!window)
    {
        return;
    }

    NOTIFYICONDATA data = { 0 };

    data.cbSize = sizeof(data);
    data.hWnd = window;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_GUID;
    data.uCallbackMessage = TRAY_MESSAGE;
    data.hIcon = (HICON)LoadImage(instance, "AppIcon", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    data.uVersion = NOTIFYICON_VERSION_4;
    data.guidItem = __uuidof(USBSyncGuid);

    snprintf(data.szTip, 128, USBSYNC_APP_ID);

    if (!Shell_NotifyIcon(NIM_ADD, &data))
    {
        return;
    }

    if (!Shell_NotifyIcon(NIM_SETVERSION, &data))
    {
        return;
    }

    MSG message;

    while (GetMessage(&message, window, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}
