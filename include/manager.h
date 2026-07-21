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
    SERVICE_STATUS_HANDLE statusHandle = nullptr;
    HCMNOTIFICATION notifyContext;
};

struct USBSyncManager
{
    static USBSyncManager* init();

    ~USBSyncManager();

    bool createService() const;
    bool deleteService() const;

    void startService() const;

private:
    USBSyncManager(SC_HANDLE manager);

    SC_HANDLE manager;

};
