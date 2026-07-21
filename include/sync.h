#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <time.h>

#include <Windows.h>

struct SyncTask
{
    SyncTask(const GUID& guid, const std::string& source, const std::string& dest);

    bool matches(const GUID& guid) const;

    void signalAbort();

private:
    const GUID guid;

    const std::string source;
    const std::string dest;

    std::thread thread;

    std::atomic<bool> abort = false;

};

struct USBSync
{
    ~USBSync();

    void beginSync(const GUID& guid, const std::string& name, const std::string& source);
    void stopSync(const GUID& guid);
    void stopAllSync();

private:
    SyncTask* task = nullptr;

};
