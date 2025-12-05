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

#include "TimedEvent.hpp"
#include "ErrorPage.hpp"

using namespace std::filesystem;
using namespace Timed;

struct OngoingFileTransfer;

namespace TransferRegistry
{
    using TransferId = int;
    std::mutex Mutex;
    std::map<TransferId, std::unique_ptr<OngoingFileTransfer>> Transfers;
}

struct OngoingFileTransfer
{
    // TODO: std::weak_ptr<Session*> m_Session;
    path m_Path;
    std::ofstream m_OutStream;
    std::unique_ptr<TimedEvent> m_Event;
    const TransferRegistry::TransferId m_Id;

    size_t m_SizeTotal = 0;
    size_t m_SizeLeft = 0;

    OngoingFileTransfer() = default;

    OngoingFileTransfer(TransferRegistry::TransferId id, path path, size_t sizeTotal) : 
        m_OutStream(path, std::ios_base::binary),
        m_SizeTotal(sizeTotal),
        m_SizeLeft(sizeTotal),
        m_Id(id),
        m_Event(std::make_unique<TimedEvent>(
            std::chrono::file_clock::now() + std::chrono::hours(1),
            [transfer = this]()
            {
                using namespace TransferRegistry;

                std::lock_guard guard(Mutex);

                auto it = Transfers.find(transfer->m_Id);
                Transfers.erase(it);
            }))
    {
        m_Event->AddEvent();
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
};

namespace TransferRegistry 
{
    std::pair<TransferId, OngoingFileTransfer&> AddTransfer(path path, size_t size)
    {
        std::scoped_lock lock(Mutex);

        TransferId id;
        do
        {
            id = rand();
        }
        while (id == 0 || Transfers.count(id) > 0);

        Transfers.insert(std::make_pair(id, std::make_unique<OngoingFileTransfer>(id, path, size)));
        return std::pair<TransferId, OngoingFileTransfer&>(id, *Transfers[id]);
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
    auto& transfer = *it->second.get();

    size_t step = std::min(request.m_Body.size(), transfer.m_SizeLeft);

    transfer.m_OutStream.write(request.m_Body.data(), step);
    transfer.m_SizeLeft -= step;

    transfer.m_OutStream.flush();
    if (transfer.m_SizeLeft == 0)
    {
        transfer.m_Event->Cancel();
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
