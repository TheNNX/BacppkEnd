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

#define CLIENT_BUFFER_SIZE 8192

#include "Html.hpp"
#include "inet-socket-wrapper.h"

#include "IndexPage.hpp"
#include "ErrorPage.hpp"

using namespace InetSocketWrapper;

struct FileResponder
{
    std::string m_File;
    std::string m_MimeType;

    FileResponder(
        const std::string& file, 
        const std::string& mimeType = "") :
        m_File(file), m_MimeType(mimeType) 
    {
        if (mimeType == "")
        {
            m_MimeType = "application/octet-stream";

            const std::map<std::string, std::string> mimeMapping = 
            {
                { ".png", "image/png" },
                { ".jpg", "image/jpeg" },
                { ".jpeg", "image/jpeg" },
                { ".txt", "text/plain" },
                { ".c", "text/plain" },
                { ".cpp", "text/plain" },
                { ".h", "text/plain" },
                { ".hpp", "text/plain" },
                { "Makefile", "text/plain" },
                { "makefile", "text/plain" },
                { ".html", "text/html" },
                { ".htm", "text/html" },
                { ".zip", "application/zip" }
            };

            auto toupper = [](std::string str) -> std::string
                { 
                    std::transform(str.begin(), str.end(), str.begin(), ::toupper); 
                    return str;
                };

            for (auto kv : mimeMapping)
            {
                if (toupper(file).ends_with(toupper(kv.first)))
                {
                    m_MimeType = kv.second;
                    break;
                }
            }
        }
    }

    HttpResponse operator()(const Request& request)
    {
        std::ifstream file(m_File, std::ios::binary | std::ios::ate);
        std::string content = "";

        int sizeLeft = file.tellg();

        if (sizeLeft < 0)
        {
            throw std::runtime_error("FileResponder couldn't locate file " + this->m_File);
        }

        file.seekg(file.beg);
        auto method = request.m_Method;

        struct Promise : ContentPromise
        {
            const Request& request;
            int sizeLeft;
            std::ifstream file;

            Promise(const Request& request, int sizeLeft, std::ifstream&& file) :
                request(request), sizeLeft(sizeLeft), file(std::move(file)) {}

            std::string Fulfill() override
            {
                char buffer[CLIENT_BUFFER_SIZE] = { 0 };
                std::string result = "";

                if (request.m_Method == "GET")
                {
                    while (sizeLeft > 0)
                    {
                        auto currentSize = std::min(sizeLeft, CLIENT_BUFFER_SIZE);
                        file.read(buffer, currentSize);
                        sizeLeft -= currentSize;
                        
                        result += std::string(buffer, buffer + currentSize);
                    }
                }

                return result;
            }
        };

        auto resp = HttpResponse(std::make_unique<Promise>(request, sizeLeft, std::move(file)), 200, m_MimeType);
        resp.m_Headers["Content-Length"] = std::to_string(sizeLeft);
        return resp;
    }
};

using Responder = std::function<HttpResponse(const Request& request)>;

struct HttpService;
struct Alias
{
    std::string m_To;
    HttpService& m_Service;

    Alias(HttpService& service, const std::string& to) : 
        m_To(to), m_Service(service) {}

    HttpResponse operator()(const Request& request);
};

struct Redirect
{
    std::string m_To;

    Redirect(const std::string& to) : m_To(to) {}

    HttpResponse operator()(const Request& request)
    {
        HttpResponse response("Moved to " + m_To, 301);
        response.m_Headers["Location"] = m_To;
        return response;
    }
};

std::atomic<int> g_NumThreads = 0;

struct HttpService
{
    InetSocket m_ServerSocket;
    std::string m_Name;
    std::map<std::string, std::map<std::string, Responder>> m_Responders;
    std::map<std::string, Responder> m_FallbackResponders = 
    { 
        { "GET", ErrorPage(404) },
    };
    Responder m_GeneralFallbackResponder = ErrorPage(404);
    std::unique_ptr<SslContext> m_SslContext = nullptr;

    HttpService(const std::string& interfce, uint16_t port = 80, InternetProtocol protocol = IPv4) :
        m_ServerSocket(protocol, TCP)
    {
        SocketAddress addr = { interfce.c_str(), port};

        std::cout << "[*] Binding... ";
        if (!m_ServerSocket.Bind(addr))
        {
            std::cerr << "ERROR" << std::endl << "[!!] Bind failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        std::cout << "OK" << std::endl;

        m_ServerSocket.Listen(10);

        std::cout << "[*] Listening on [" << addr.host << ':' << addr.port << ']' << std::endl;
    }

    std::thread Run()
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

                        this->HandleClient({ Connection{ std::move(clientSocket), m_SslContext } });
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

    void HandleClient(Request&& request)
    {
        static const auto getResponse = [&](const Request& request) -> std::string
            {
                HttpResponse response = HttpResponse("", 500, "text/plain");

                auto spaceFirst = request.m_Body.find(' ');
                if (spaceFirst != std::string::npos)
                {
                    std::string method = std::string(
                        request.m_Body.begin(),
                        request.m_Body.begin() + spaceFirst);

                    std::transform(method.begin(), method.end(), method.begin(), ::toupper);

                    std::string skipMethod = std::string(
                        request.m_Body.begin() + method.length() + 1,
                        request.m_Body.end());

                    request.m_Method = method;

                    auto it = std::find(skipMethod.begin(), skipMethod.end(), ' ');
                    std::string path = std::string(skipMethod.begin(), it);
                    std::string protocol = std::string(it + 1, skipMethod.end());

                    if (!protocol.starts_with("HTTP"))
                    {
                        return "";
                    }

                    std::cout << "[G] " << method << " Request for " + path + " using " +
                        protocol.substr(0, protocol.find("\r\n")) << "\n";

                    if (m_Responders.count(method) == 0)
                    { 
                        response = ErrorPage(501)(request);
                    }
                    else
                    {

                        auto responderIt = m_Responders.at(method).find(path);
                        if (responderIt == m_Responders.at(method).end())
                        {
                            if (m_FallbackResponders.count(method) > 0)
                            {
                                response = m_FallbackResponders.at(method)(request);
                            }
                            else
                            {
                                response = m_GeneralFallbackResponder(request);
                            }
                        }
                        else
                        {
                            try
                            {
                                response = responderIt->second(request);
                            }
                            catch (const std::runtime_error& e)
                            {
                                response = ErrorPage(500)(request);
                            }
                        }
                    }
                }
                else
                {
                    response = ErrorPage(501)(request);
                }

                if (request.m_Method == "HEAD")
                {
                    return response.GetResponseHeader();
                }
                return
                    response.GetResponseHeader() +
                    response.GetContent() + "\r\n";
            };

        struct WorkerCounterAcquirer
        {
            WorkerCounterAcquirer()
            {
                g_NumThreads++;
            }

            ~WorkerCounterAcquirer()
            {
                g_NumThreads--;
            }
        };

        auto workerFunction = [](Request&& requestR)
            {
                Request request = Request(std::move(requestR));
                std::string data = "";
                WorkerCounterAcquirer acquirer;

                std::cout << g_NumThreads << " threads started\n";

                auto headersEnd = std::string::npos;
                size_t contentLeft = 0;

                std::map<std::string, std::string> headerMap;

                while (
                    headersEnd == std::string::npos &&
                    !request.m_Connection.Bad() &&
                    !request.m_Connection.Eof())
                {
                    data += request.m_Connection.ReceiveString();
                    if (headersEnd == std::string::npos)
                    {
                        headersEnd = data.find("\r\n\r\n");
                    }
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

                        auto trim = [](std::string value)
                            {
                                while (value.starts_with(' '))
                                {
                                    value = value.substr(1);
                                }
                                while (value.ends_with(' '))
                                {
                                    value = value.substr(0, value.length() - 1);
                                }

                                return value;
                            };

                        name = trim(name);
                        value = trim(value);

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
                    std::string got = request.m_Connection.ReceiveString(std::min(contentLeft, (size_t)4096));
                    contentLeft -= got.length();
                    data += got;
                }

                request.m_Body = data;
                request.m_RequestHeaders = std::move(headerMap);
                std::string result = getResponse(request);
                request.m_Connection.SendString(result);
            };

        std::thread workerThread = std::thread(workerFunction, std::move(request));
        workerThread.detach();
    }
};

struct LoginApi
{
    HttpResponse operator()(const Request& request)
    {
        return ErrorPage(401)(request);
    }
};

int main()
{
    HttpService httpsService = HttpService("0.0.0.0", 3443);
    httpsService.m_SslContext = std::make_unique<SslContext>(
        "server.crt",
        "server.key");
    httpsService.m_Responders["GET"] =
    {
        { "/", IndexPage() },
        { "/index.html", Alias(httpsService, "/") },
        { "/robots.txt", FileResponder("robots.txt", "text/plain") },
        { (std::string) StylesheetPath, Styler() }
    };
    httpsService.m_Responders["POST"] =
    {
        { "/api/login", LoginApi() }
    };
    std::thread t1 = httpsService.Run();

    HttpService httpRedirector = HttpService("0.0.0.0", 3080);
    httpRedirector.m_Responders["GET"] =
    {
        /* TODO */
    };
    httpRedirector.m_GeneralFallbackResponder = Alias(httpRedirector, "/");
    //std::thread t2 = httpRedirector.Run();

    t1.join();
    //t2.join();

    return 0;
}

HttpResponse Alias::operator()(const Request& request)
{
    return m_Service.m_Responders.at("GET").at(m_To)(request);
}