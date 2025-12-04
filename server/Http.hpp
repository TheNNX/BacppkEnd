#pragma once
#include <string>
#include <vector>
#include <time.h>
#include <map>
#include <memory>

#include "StringHelper.hpp"

#include "InetSocketWrapper.h"
#include "Https.hpp"

namespace InetSocketWrapper
{
    class InetSocket;
}

struct ResourceIdentifier
{
    std::string m_Path;
    std::map<std::string, std::vector<std::string>> m_Query;

    std::vector<std::string> GetPathParts() const
    {
        return SplitString(m_Path, '/');
    }

    ResourceIdentifier(const std::string& ri);
    ResourceIdentifier() = default;
};

struct Connection
{
    Connection(
        InetSocketWrapper::InetSocket&& socket,
        InetSocketWrapper::SocketAddress clientAddress,
        std::unique_ptr<SslContext>& sslContext) :
        m_ClientSocket(std::move(socket)),
        m_ClientAddress(clientAddress)
    {
        if (sslContext != nullptr)
        {
            m_SslConnection = std::make_unique<SslConnection>(*sslContext, m_ClientSocket);
        }
    }
    Connection(Connection&& connection) = default;

    bool Bad();
    bool Eof();
    void SendString(const std::string& str);
    std::string ReceiveString(size_t len = 8192);
    InetSocketWrapper::SocketAddress GetAddress() 
    {
        return this->m_ClientAddress;
    }
private:
    InetSocketWrapper::InetSocket m_ClientSocket;
    const InetSocketWrapper::SocketAddress m_ClientAddress;
    std::unique_ptr<SslConnection> m_SslConnection = nullptr;
};

struct Request
{
    Request(
        Connection&& connection) :
        m_Connection(std::move(connection)), m_Data("")
    {
    }

    std::string m_Data;
    Connection m_Connection;
    std::string_view m_Body;
    ResourceIdentifier m_ResourceId;
    std::string m_Protocol;
    std::map<std::string, std::string> m_RequestHeaders;
    std::string m_Method = "";
};

inline std::string StringifyHttpCode(int code)
{
    switch (code)
    {
    case 100:
        return "Continue";
    case 101:
        return "Switching Protocols";
    case 102:
        return "Processing";
    case 103:
        return "Early Hints";
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 202:
        return "Accepted";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 303:
        return "See Other";
    case 304:
        return "Not Modified";
    case 307:
        return "Temporary Redirect";
    case 308:
        return "Permament Redirect";
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 402:
        return "Payment Required";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 501:
        return "Not Implemented";
    case 502:
        return "Bad Gateway";
    case 503:
        return "Service Unavaible";
    default:
        return "<???>";
    }
}

class ContentPromise
{
public:
    virtual ~ContentPromise() = default;
    virtual std::string Fulfill() = 0;
};

class StrContentPromise : public ContentPromise
{
    std::string m_Content;

public:
    StrContentPromise(const std::string& str) : m_Content(str) {}
    ~StrContentPromise() = default;

    std::string Fulfill() override
    {
        return m_Content;
    }
};

class HttpResponse
{
private:
    std::unique_ptr<ContentPromise> m_ContentPromise;
    int m_HttpCode;
public:
    std::map<std::string, std::string> m_Headers;

    template<typename T>
    inline HttpResponse(T&& content, int httpCode, const std::string& contentType) :
        HttpResponse(std::forward<T>(content), httpCode)
    {
        m_Headers["Content-Type"] = contentType;
        m_Headers["Connection"] = "close";
    }

    inline HttpResponse(const std::string& content, int httpCode) : 
        m_ContentPromise(std::make_unique<StrContentPromise>(content)), 
        m_HttpCode(httpCode) {}

    template<std::convertible_to<std::unique_ptr<ContentPromise>> T >
    inline HttpResponse(T&& content, int httpCode) : 
        m_ContentPromise(std::move(content)), 
        m_HttpCode(httpCode) {}

    inline std::string GetContent()
    {
        return this->m_ContentPromise->Fulfill();
    }

    inline std::string GetContentType()
    {
        if (m_Headers.contains("Content-Type"))
        {
            return m_Headers["Content-Type"];
        }
        return "";
    }

    inline int GetHttpCode()
    {
        return this->m_HttpCode;
    }

    inline std::string GetResponseHeader()
    {
        std::string result =
            "HTTP/1.1 " + std::to_string(GetHttpCode()) + " " + StringifyHttpCode(GetHttpCode()) + "\r\n";

        for (auto kv : m_Headers)
        {
            result += kv.first + ": " + kv.second + "\r\n";
        }
        result += "\r\n";

        return result;
    }

    inline std::string GetResponse()
    {
        return GetResponseHeader() + GetContent() + "\r\n";
    }
};

inline bool Connection::Bad()
{
    if (this->m_SslConnection != nullptr && this->m_SslConnection->Bad())
    {
        return true;
    }
    return this->m_ClientSocket.Bad();
}

inline bool Connection::Eof()
{
    return this->m_ClientSocket.Eof();
}

inline void Connection::SendString(const std::string& str)
{
    if (this->m_SslConnection == nullptr)
    {
        m_ClientSocket.SendString(str);
        return;
    }

    m_SslConnection->Write(str);
}

inline std::string Connection::ReceiveString(size_t len)
{
    if (this->m_SslConnection == nullptr)
    {
        return m_ClientSocket.ReceiveString(len);
    }
    return this->m_SslConnection->Read(len);
}