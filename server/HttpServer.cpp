#include "Http.hpp"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <functional>
#include <math.h>
#include <thread>
#include <assert.h>

#include "Html.hpp"
#include "InetSocketWrapper.h"

#include "HttpServer.hpp"
#include "FileResponder.hpp"

#include "IndexPage.hpp"
#include "ErrorPage.hpp"
#include "UploadApi.hpp"

HttpService::HttpService(const std::string& interfce, uint16_t port, InternetProtocol protocol) :
    m_ServerSocket(protocol, TCP)
{
    SocketAddress addr = { interfce.c_str(), port };

    switch (port)
    {
    case 80:
        m_Name = "HTTP";
        break;
    case 443:
        m_Name = "HTTPS";
        break;
    default:
        m_Name = ":" + std::to_string(port);
    }

    std::cout << "[*] Binding... ";
    if (!m_ServerSocket.Bind(addr))
    {
        std::cerr << "ERROR" << std::endl << "[!!] Bind failed" << std::endl;
        throw std::runtime_error("Failed to create service");
    }
    std::cout << "OK" << std::endl;

    m_ServerSocket.Listen(10);
    std::cout << "[*] Listening on [" << addr.host << ':' << addr.port << ']' << std::endl;
}

std::thread HttpService::Run()
{
    auto runService = [&]()
        {
            std::cout << "[S] Service '" << m_Name << "' initializing\n";

            while (true)
            {
                try
                {
                    SocketAddress clientAddress;

                    auto clientSocket = m_ServerSocket.AcceptConnection(clientAddress);
                    std::cout << "[+] New connection: [" << clientAddress.host << ':';
                    std::cout << clientAddress.port << ']' << std::endl;

                    HttpClientWorker worker(Connection
                                            {
                                                std::move(clientSocket), 
                                                clientAddress, m_SslContext
                                            },
                                            *this);
                }
                catch (const std::runtime_error& rerror)
                {
                    std::cerr << "[E] " << rerror.what() << "\n";
                }
                catch (...)
                {
                    std::cerr << "[E] Unknown error.\n";
                }
            }
        };

    return std::thread(runService);
}

HttpResponse HttpService::GetResponse(const Request& request) const
{
    assert(request.m_ResourceId.GetPathParts().size() > 0);
    std::vector<std::string> pathParts = request.m_ResourceId.GetPathParts();

    std::cout << "[G] " << request.m_Method << " Request for " + request.m_ResourceId.m_Path + " using " << request.m_Protocol << "\n";

    if (m_Responders.count(request.m_Method) == 0)
    {
        return ErrorPage(501)(request);
    }

    if (m_Responders.at(request.m_Method).count(pathParts[0]) == 0)
    {
        return ErrorPage(404)(request);
    }

    auto responderIt = m_Responders.at(request.m_Method).find(pathParts[0]);

    const Responder* responder = responderIt == m_Responders.at(request.m_Method).end() ?
        nullptr : 
        &responderIt->second;

    for (size_t i = 1; i < pathParts.size() && responder != nullptr; i++)
    {
        responder = responder->GetChild(pathParts[i]);
    }

    if (responder == nullptr)
    {
        if (m_FallbackResponders.count(request.m_Method) > 0)
        {
            return m_FallbackResponders.at(request.m_Method)(request);
        }

        return m_GeneralFallbackResponder(request);
    }

    try
    {
        return responder->m_Respond(request);
    }
    catch (const std::runtime_error& e)
    {
        return ErrorPage(500)(request);
    }
}

ResourceIdentifier::ResourceIdentifier(const std::string& ri)
{
    auto queryIt = std::find(ri.begin(), ri.end(), '?');
 
    std::string queryString;

    m_Path = std::string(ri.begin(), queryIt);
    if (queryIt == ri.end())
    {
        return;
    }

    queryString = std::string(queryIt + 1, ri.end());

    auto queryParts = SplitString(queryString, '&');
    for (auto& queryPart : queryParts)
    {
        std::string key, value;
        auto eqSignIt = std::find(queryPart.begin(), queryPart.end(), '=');

        if (eqSignIt == queryPart.end())
        {
            value = "";
            key = queryPart;
        }
        else 
        {
            value = std::string(eqSignIt + 1, queryPart.end());
            key = std::string(queryPart.begin(), eqSignIt);
        }

        m_Query[key].push_back(value);
    }

    std::cout << m_Query.size() << " query keys\n";
}

void HttpClientWorker::WorkerFunction(Connection&& originalConnection)
{
    Connection connection = std::move(originalConnection);
    std::string data = "";
    WorkerCounterAcquirer acquirer;

    std::cout << WorkerCounterAcquirer::GetNumberOfThreadsRef() << " threads started\n";

    auto headersEnd = std::string::npos;
    size_t contentLeft = 0;

    std::map<std::string, std::string> headerMap;

    while (headersEnd == std::string::npos &&
        !connection.Bad() &&
        !connection.Eof())
    {
        data += connection.ReceiveString();
        if (headersEnd == std::string::npos)
        {
            headersEnd = data.find("\r\n\r\n");
        }
    }

    if (headersEnd == std::string::npos)
    {
        /* Ignore empty requests, browsers establish connections "in advance" when
           typing to decrease load times. When the address later changes, the
           connection is closed not having sent any data. */
        if (data.length() == 0)
        {
            return;
        }
        std::cerr << "Got an invalid request from " << connection.GetAddress().ToString() << "\n";
        return;
    }

    std::string headers = data.substr(0, headersEnd);

    std::stringstream sstream(headers);
    std::string line;
    while (std::getline(sstream, line))
    {
        auto pos = line.find_first_of(':');
        if (pos != std::string::npos)
        {
            std::string name = line.substr(0, pos);
            std::string value = std::string(line.begin() + pos + 1, line.end());

            name = Trim(name);
            value = Trim(value);

            headerMap[name] = value;

            if (name == "Content-Length")
            {
                std::stringstream(value) >> contentLeft;
            }
        }
    }

    contentLeft -= data.length() - headers.length() - 4;
    while (contentLeft > 0)
    {
        std::string got = connection.ReceiveString(std::min(contentLeft, (size_t)4096));
        contentLeft -= got.length();
        data += got;
    }

    auto spaceFirst = data.find(' ');
    if (spaceFirst == std::string::npos)
    {
        return;
    }
    std::string method = std::string(
        data.begin(),
        data.begin() + spaceFirst);

    std::transform(method.begin(), method.end(), method.begin(), ::toupper);

    std::string skipMethod =
        std::string(
            data.begin() + method.length() + 1,
            data.end());

    auto it = std::find(skipMethod.begin(), skipMethod.end(), ' ');

    ResourceIdentifier resourceId(std::string(skipMethod.begin(), it));
    std::string protocol = std::string(it + 1, skipMethod.end());

    if (!protocol.starts_with("HTTP"))
    {
        return;
    }

    protocol = protocol.substr(0, protocol.find("\r\n"));

    Request request(std::move(connection));

    request.m_Data = data;
    request.m_ResourceId = resourceId;
    request.m_Method = method;
    request.m_Protocol = protocol;
    request.m_RequestHeaders = std::move(headerMap);
    request.m_Body = std::string_view(data.begin() + headersEnd + 4, data.end());

    HttpResponse response = m_Service.GetResponse(request);
    if (request.m_Method == "HEAD")
    {
        request.m_Connection.SendString(response.GetResponseHeader());
    }
    else
    {
        request.m_Connection.SendString(response.GetResponse());
    }
}

int main()
{
    //HttpService httpsService = HttpService("0.0.0.0", 43);
    //httpsService.m_SslContext = std::make_unique<SslContext>("server.crt",
    //                                                         "server.key");
    //httpsService.m_Responders["GET"] =
    //{
    //    { "/", IndexPage() },
    //    { "/index.html", Alias(httpsService, "/") },
    //    { "/robots.txt", FileResponder("robots.txt", "text/plain") },
    //    { (std::string) StylesheetPath, Styler() }
    //};
    //httpsService.m_Responders["POST"] =
    //{
    //    { "/api/login", LoginApi() }
    //};
    //std::thread t1 = httpsService.Run();

    HttpService httpService = HttpService("0.0.0.0", 80);
    httpService.m_Responders["GET"] =
    {
        { "/", IndexPage() },
        { "/index.html", Alias(httpService, "/") }
    };

	httpService.m_Responders["POST"] = 
	{
		{ "/upload", UploadApi() },
        { "/uploadFile", UploadFileApi() },
	};
    httpService.m_GeneralFallbackResponder = Alias(httpService, "/");

    std::thread t2 = httpService.Run();

    //t1.join();
    t2.join();

    return 0;
}

HttpResponse Alias::operator()(const Request& request)
{
    return m_Service.m_Responders.at("GET").at(m_To)(request);
}
