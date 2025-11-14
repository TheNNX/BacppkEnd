#include "UploadApi.hpp"

#include <mutex>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <assert.h>
#include <queue>

#include "ErrorPage.hpp"

using namespace std::filesystem;

struct OngoingFileTransfer
{
    // TODO: std::weak_ptr<Session*> m_Session;
    path m_Path;
    std::ofstream m_OutStream;

    size_t m_SizeTotal = 0;
    size_t m_SizeLeft = 0;

    /* TODO: create a worker to trim this */
    std::chrono::time_point<std::chrono::file_clock> m_ExpirationDate;

    OngoingFileTransfer() = default;

    OngoingFileTransfer(path path, size_t sizeTotal) : 
        m_OutStream(path, std::ios_base::binary),
        m_SizeTotal(sizeTotal),
        m_SizeLeft(sizeTotal)
    {
        m_ExpirationDate = std::chrono::file_clock::now() + std::chrono::hours(1);
    }

    void Append(std::string_view data)
    {
        size_t writeSize = data.size();
        if (writeSize > m_SizeLeft)
        {
            writeSize = m_SizeLeft;
        }
        m_SizeLeft -= writeSize;

        m_OutStream.write(data.data(), data.size());
    }

    bool IsExpired() const
    {
        return std::chrono::file_clock::now() > m_ExpirationDate;
    }
};

namespace TransferRegistry 
{
    using TransferId = int;
    std::map<TransferId, OngoingFileTransfer> Transfers;
    std::mutex Mutex;

    auto GetNextTimeout()
    {
        std::lock_guard lock(Mutex);

        /* If there are no ongnoing transfers, wait for one minute. 
           TODO: This should probably wait on a semaphore instead,
           though it's not very critical code. */
        if (Transfers.size() == 0)
        {
            return std::chrono::file_clock::now() + std::chrono::minutes(1);
        }

        decltype(OngoingFileTransfer::m_ExpirationDate) nextTimeout =
            Transfers.begin()->second.m_ExpirationDate;

        for (auto& transfer : Transfers)
        {
            if (transfer.second.m_ExpirationDate < nextTimeout)
            {
                nextTimeout = transfer.second.m_ExpirationDate;
            }
        }

        return nextTimeout;
    }

    std::thread CleanupThread = []() -> std::thread&&
        {
            std::thread t([]()
                {
                    while (true)
                    {
                        
                        std::this_thread::sleep_until(GetNextTimeout());

                        std::queue<TransferId> toRemove;

                        std::lock_guard lock(Mutex);
                        for (auto it = Transfers.begin(); it != Transfers.end(); it++)
                        {
                            if (it->second.IsExpired())
                            {
                                toRemove.push(it->first);
                            }
                        }

                        while (toRemove.empty() == false)
                        {
                            auto key = toRemove.front();
                            Transfers.erase(key);
                            toRemove.pop();
                        }
                    }
                });

            t.detach();
            return std::move(t);
        }();

    std::pair<TransferId, OngoingFileTransfer&> AddTransfer(path path, size_t size)
    {
        std::scoped_lock lock(Mutex);

        TransferId id;
        do
        {
            id = rand();
        } 
        while (id == 0 || Transfers.count(id) > 0);
    
        Transfers[id] = OngoingFileTransfer(path, size);
        return std::pair<TransferId, OngoingFileTransfer&>(id, Transfers[id]);
    }
};

HttpResponse UploadFileApi::operator()(const Request& request)
{
    std::scoped_lock<std::mutex> lock(TransferRegistry::Mutex);

    if (request.m_ResourceId.m_Query.count("id") == 0)
    {
        return ErrorPage(400)(request);
    }

    int id = std::stoi(request.m_ResourceId.m_Query.at("id")[0]);

    /* TODO: return 403 if session is not the owner for this id */
    if (TransferRegistry::Transfers.count(id) == 0)
    {
        return ErrorPage(403)(request);
    }

    auto it = TransferRegistry::Transfers.find(id);
    auto& transfer = it->second;
    if (transfer.IsExpired())
    {
        TransferRegistry::Transfers.erase(it);
    }

    size_t step = std::min(request.m_Body.size(), transfer.m_SizeLeft);

    transfer.m_OutStream.write(request.m_Body.data(), step);
    transfer.m_SizeLeft -= step;

    transfer.m_OutStream.flush();
    if (transfer.m_SizeLeft == 0)
    {
        TransferRegistry::Transfers.erase(it);
    }

    /* TODO: error handling */
    return HttpResponse("", 200);
}

HttpResponse UploadApi::operator()(const Request& request)
{
    std::cout << "Invoking upload api with body: " + std::string(request.m_Body) + "\n";
    std::stringstream stream = std::stringstream(std::string(request.m_Body));
    std::string line = "";

    struct File 
    {
        path m_Path;
        int m_Size;
    };

    std::vector<File> files;

    std::string response;
    while (stream.eof() == false)
    {
        int size;
        std::string name;

        std::getline(stream, line);
        std::stringstream lineStream(line);
        std::cout << "Got line '" << line << "', length = " + std::to_string(line.size()) + "\n";

        if (line == "\r" || line == "")
        {
            std::cout << "Breaking\n";
            break;
        }

        lineStream >> size;
        while (std::isspace(lineStream.peek()))
        {
            lineStream.get();
        }
        std::getline(lineStream, name);

        path filePath = m_ServerUploadDirectory / path(name).filename();
        bool doesExist = exists(filePath);

        TransferRegistry::TransferId id = 0;
        if (!doesExist)
        {
            id = TransferRegistry::AddTransfer(filePath, size).first;
        }

        response +=
            std::to_string(id) + std::string(";") +
            std::to_string(size) + std::string(";") + filePath.string() + 
            std::string(";") + ("false\0true" + (6 * doesExist)) + "\n";
    }

    std::cout << "Response: " << response << "\n";
    return HttpResponse(response, 200);
}
