#include "manager.h"

static constexpr char* serviceName = "usbsync";

DWORD WINAPI handleControl(DWORD control, DWORD type, LPVOID data, LPVOID context)
{
    switch (control)
    {
        case SERVICE_CONTROL_INTERROGATE:
            return NO_ERROR;

        case SERVICE_CONTROL_STOP:
        {
            HandlerContext* handlerContext = (HandlerContext*)context;

            SERVICE_STATUS status;

            status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
            status.dwCurrentState = SERVICE_STOPPED;
            status.dwControlsAccepted = 0;
            status.dwCheckPoint = 0;
            status.dwWaitHint = 0;

            CONFIGRET result = CM_Unregister_Notification(handlerContext->notifyContext);

            if (result == CR_SUCCESS)
            {
                status.dwWin32ExitCode = NO_ERROR;
            }

            else
            {
                status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
                status.dwServiceSpecificExitCode = result;
            }

            SetServiceStatus(handlerContext->statusHandle, &status);

            delete handlerContext;

            return NO_ERROR;
        }

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

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

void WINAPI serviceMain(DWORD argc, LPTSTR* argv)
{
    HandlerContext* context = new HandlerContext();

    context->statusHandle = RegisterServiceCtrlHandlerEx(serviceName, &handleControl, context);

    if (!context->statusHandle)
    {
        delete context;

        return;
    }

    SERVICE_STATUS status;

    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;

    CM_NOTIFY_FILTER filter;

    filter.cbSize = sizeof(CM_NOTIFY_FILTER);
    filter.Flags = 0;
    filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    filter.Reserved = 0;
    filter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_VOLUME;

    CONFIGRET result = CM_Register_Notification(&filter, context, handleNotify, &context->notifyContext);

    if (result == CR_SUCCESS)
    {
        status.dwCurrentState = SERVICE_RUNNING;
        status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        status.dwWin32ExitCode = NO_ERROR;
    }

    else
    {
        status.dwCurrentState = SERVICE_STOPPED;
        status.dwControlsAccepted = 0;
        status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        status.dwServiceSpecificExitCode = result;
    }

    SetServiceStatus(context->statusHandle, &status);
}

HandlerContext::HandlerContext() :
    sync(new USBSync()) {}

HandlerContext::~HandlerContext()
{
    delete sync;
}

void USBSyncManager::startService()
{
    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { serviceName, serviceMain },
        { nullptr, nullptr }
    };

    StartServiceCtrlDispatcher(dispatchTable);
}
