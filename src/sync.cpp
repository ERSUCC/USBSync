#include "sync.h"

SyncTask::SyncTask(const GUID& guid, const std::string& name, const std::string& source, const std::string& dest) :
    guid(guid), name(name), source(source), dest(dest)
{
    thread = std::thread([=]()
    {
        USBSyncToast::initToast();
        USBSyncToast::displayToast("Sync started for " + name);

        std::filesystem::create_directories(dest);

        for (const std::filesystem::directory_entry entry : std::filesystem::recursive_directory_iterator(source))
        {
            if (abort)
            {
                USBSyncToast::displayToast("Sync aborted for " + name);

                break;
            }

            if (!entry.is_directory())
            {
                const std::filesystem::path target = dest / std::filesystem::relative(entry.path(), source);

                std::filesystem::create_directories(target.parent_path());

                CopyFile(entry.path().string().c_str(), target.string().c_str(), false);
            }
        }

        if (!abort)
        {
            MoveFile(dest.c_str(), dest.substr(0, dest.size() - 11).c_str());

            USBSyncToast::displayToast("Sync completed for " + name);
        }

        USBSyncToast::deinitToast();
    });
}

bool SyncTask::matches(const GUID& guid) const
{
    return this->guid == guid;
}

void SyncTask::signalAbort()
{
    abort = true;

    if (thread.joinable())
    {
        thread.join();
    }
}

USBSync::~USBSync()
{
    stopAllSync();
}

void USBSync::beginSync(const GUID& guid, const std::string& name, const std::string& source)
{
    stopAllSync();

    const time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char dateTime[20];

    strftime(dateTime, sizeof(char) * 20, "%m-%d-%Y-%H-%M-%S", std::localtime(&time));

    dateTime[19] = '\0';

    char dest[MAX_PATH];

    snprintf(dest, MAX_PATH, "%s\\%s-%s_UNFINISHED", backupRoot, name.c_str(), dateTime);

    task = new SyncTask(guid, name, source, dest);
}

void USBSync::stopSync(const GUID& guid)
{
    if (task && task->matches(guid))
    {
        task->signalAbort();

        delete task;
    }
}

void USBSync::stopAllSync()
{
    if (task)
    {
        task->signalAbort();

        delete task;
    }
}
