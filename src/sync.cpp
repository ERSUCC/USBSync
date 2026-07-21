#include "sync.h"

SyncTask::SyncTask(const std::string& source, const std::string& dest) :
    source(source), dest(dest)
{
    thread = std::thread([=]()
    {
        std::filesystem::create_directories(dest);

        for (const std::filesystem::directory_entry entry : std::filesystem::recursive_directory_iterator(source))
        {
            if (abort)
            {
                return;
            }

            if (!entry.is_directory())
            {
                const std::filesystem::path target = dest / std::filesystem::relative(entry.path(), source);

                std::filesystem::create_directories(target.parent_path());

                CopyFile(entry.path().string().c_str(), target.string().c_str(), false);
            }
        }

        MoveFile(dest.c_str(), dest.substr(0, dest.size() - 11).c_str());
    });
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
    stopSync();
}

void USBSync::beginSync(const std::string& source)
{
    stopSync();

    const time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char dateTime[20];

    strftime(dateTime, sizeof(char) * 20, "%m-%d-%Y-%H-%M-%S", std::localtime(&time));

    dateTime[19] = '\0';

    task = new SyncTask(source, std::string("C:\\ProgramData\\USBSync\\Backups\\") + dateTime + "_UNFINISHED");
}

void USBSync::stopSync()
{
    if (task)
    {
        task->signalAbort();

        delete task;
    }
}
