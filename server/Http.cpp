#include "Http.hpp"

#include "Connection.hpp"
#include "StringHelper.hpp"

std::map<std::string, std::vector<std::string>> ParseQueryString(const std::string& queryString)
{
    std::map<std::string, std::vector<std::string>> result;

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
            for (char& c : value)
            {
                if (c == '+')
                {
                    c = ' ';
                }
            }
            key = std::string(queryPart.begin(), eqSignIt);
            for (char& c : key)
            {
                if (c == '+')
                {
                    c = ' ';
                }
            }
        }

        result[key].push_back(value);
    }

    return result;
}

std::map<std::string, std::string> ParseQueryStringUnique(const std::string& queryString)
{
    auto nonUnique = ParseQueryString(queryString);
    std::map<std::string, std::string> result;

    for (auto& it : nonUnique)
    {
        if (it.second.size() != 1)
        {
            throw DuplicateQueryKeyException("Duplicate query key " + it.first);
        }

        result[it.first] = it.second[0];
    }

    return result;
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

    m_Query = ParseQueryString(queryString);
}

std::vector<std::string> ResourceIdentifier::GetPathParts() const
{
    return SplitString(m_Path, '/');
}

std::string StringifyHttpCode(int code)
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
    case 500:
        return "Internal Server Error";
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

std::string HttpResponse::GetResponseHeader() const
{
    std::string result =
        "HTTP/1.1 " + std::to_string(GetHttpCode()) + " " + 
        StringifyHttpCode(GetHttpCode()) + "\r\n";

    for (auto kv : m_Headers)
    {
        result += kv.first + ": " + kv.second + "\r\n";
    }
    result += "\r\n";

    return result;
}

std::string HttpResponse::GetContentType() const
{
    if (m_Headers.contains("Content-Type"))
    {
        return m_Headers.at("Content-Type");
    }
    return "";
}

std::string HttpResponse::GetContent()
{
    return this->m_ContentPromise->Fulfill();
}

std::string HttpResponse::GetResponse()
{
    return GetResponseHeader() + GetContent() + "\r\n";
}

Request::Request(Connection&& connection) :
    m_Connection(std::move(connection)),
    m_Data("")
{
}

std::map<std::string, std::string> Request::GetCookies() const
{
    std::map<std::string, std::string> result;

    if (!m_RequestHeaders.contains("Cookie"))
    {
        return result;
    }

    const std::string& cookiesHeader = m_RequestHeaders.at("Cookie");
    auto cookies = SplitString(cookiesHeader, ';');

    for (const auto& cookie : cookies)
    {
        auto kv = SplitString(cookie, '=');

        /* Skip invalid cookies */
        if (kv.size() != 2)
        {
            continue;
        }

        result[kv[0]] = kv[1];
    }

    return result;
}