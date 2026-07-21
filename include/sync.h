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
    static USBSync* getInstance();

    static void destroyInstance();

    void beginSync(const std::string& source);
    void stopSync();

private:
    ~USBSync();

    static USBSync* instance;

    SyncTask* task = nullptr;

};
