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
    SyncTask(const std::string& source, const std::string& dest);

    void signalAbort();

private:
    const std::string source;
    const std::string dest;

    std::thread thread;

    std::atomic<bool> abort = false;

};

struct USBSync
{
    ~USBSync();

    void beginSync(const std::string& source);
    void stopSync();

private:
    SyncTask* task = nullptr;

};
