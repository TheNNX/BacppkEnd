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
#include "ErrorPage.hpp"

using namespace std::filesystem;

struct OngoingFileTransfer
{
    // TODO: std::weak_ptr<Session*> m_Session;
    path m_Path;
    std::ofstream m_OutStream;

    size_t m_SizeTotal;
    size_t m_SizeLeft;

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
    std::map<TransferId, OngoingFileTransfer> g_Transfers;
    std::mutex m_Mutex;

    std::pair<TransferId, OngoingFileTransfer&> AddTransfer(path path, size_t size)
    {
        std::scoped_lock lock(m_Mutex);

        TransferId id;
        do
        {
            id = rand();
        } 
        while (id == 0 || g_Transfers.count(id) > 0);
    
        g_Transfers[id] = OngoingFileTransfer(path, size);
        return std::pair<TransferId, OngoingFileTransfer&>(id, g_Transfers[id]);
    }
};

HttpResponse UploadFileApi::operator()(const Request& request)
{
    std::scoped_lock<std::mutex> lock(TransferRegistry::m_Mutex);

    if (request.m_ResourceId.m_Query.count("id") == 0)
    {
        return ErrorPage(400)(request);
    }

    int id = std::stoi(request.m_ResourceId.m_Query.at("id")[0]);

    /* TODO: return 403 if session is not the owner for this id */
    if (TransferRegistry::g_Transfers.count(id) == 0)
    {
        return ErrorPage(403)(request);
    }

    auto it = TransferRegistry::g_Transfers.find(id);
    auto& transfer = it->second;
    if (transfer.IsExpired())
    {
        TransferRegistry::g_Transfers.erase(it);
    }

    size_t step = std::min(request.m_Body.size(), transfer.m_SizeLeft);

    transfer.m_OutStream.write(request.m_Body.data(), step);
    transfer.m_SizeLeft -= step;

    transfer.m_OutStream.flush();
    if (transfer.m_SizeLeft == 0)
    {
        TransferRegistry::g_Transfers.erase(it);
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
