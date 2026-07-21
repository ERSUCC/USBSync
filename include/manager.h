#pragma once

#include <optional>
#include <stdio.h>
#include <string>
#include <wchar.h>

#include <Windows.h>
#include <cfgmgr32.h>

#include "sync.h"

struct HandlerContext
{
    HandlerContext();
    ~HandlerContext();

    USBSync* sync;

    SERVICE_STATUS_HANDLE statusHandle = nullptr;
    HCMNOTIFICATION notifyContext;
};

struct USBSyncManager
{
    static void startService();
};
