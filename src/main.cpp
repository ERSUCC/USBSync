#include <Windows.h>

#include "manager.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, PWSTR cmd, int show)
{
    USBSyncManager::startService();

    return 0;
}
