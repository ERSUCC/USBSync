#include <wchar.h>

#include <Windows.h>

#include "manager.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, PWSTR cmd, int show)
{
    const USBSyncManager* manager = USBSyncManager::init();

    if (wcscmp(cmd, L"create") == 0)
    {
        manager->createService();
    }

    else if (wcscmp(cmd, L"delete") == 0)
    {
        manager->deleteService();
    }

    else
    {
        manager->startService();
    }

    delete manager;

    return 0;
}
