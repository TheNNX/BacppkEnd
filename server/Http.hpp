#pragma once
#include <string>
#include <vector>
#include <time.h>
#include <map>
#include <memory>

#include "Connection.hpp"

std::string StringifyHttpCode(int code);

namespace InetSocketWrapper
{
    class InetSocket;
}

struct DuplicateQueryKeyException : public std::exception
{
    std::string m_Message;

    DuplicateQueryKeyException(const std::string& message) :
        m_Message(message)
    {

    }

    const char* what() const noexcept
    {
        return m_Message.c_str();
    }
};

std::map<std::string, std::vector<std::string>> ParseQueryString(const std::string& queryString);
std::map<std::string, std::string> ParseQueryStringUnique(const std::string& queryString);

struct ResourceIdentifier
{
    std::string m_Path;
    std::map<std::string, std::vector<std::string>> m_Query;

    std::vector<std::string> GetPathParts() const;

    ResourceIdentifier(const std::string& ri);
    ResourceIdentifier() = default;
};

struct Request
{
    Request(Connection&& connection);

    std::string m_Data;
    Connection m_Connection;
    std::string_view m_Body;
    ResourceIdentifier m_ResourceId;
    std::string m_Protocol;
    std::map<std::string, std::string> m_RequestHeaders;
    std::string m_Method = "";

    std::map<std::string, std::string> GetCookies() const;
};

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

    std::string GetContent();

    std::string GetContentType() const;

    int GetHttpCode() const
    {
        return this->m_HttpCode;
    }

    std::string GetResponseHeader() const;

    std::string GetResponse();
};