#pragma once

#include "Http.hpp"
#include "TimedEvent.hpp"

struct LoginSession;

struct SessionHandle
{
    SessionHandle(const std::string& id);
    SessionHandle() = default;

    std::string ReadProperty(const std::string& key);
    std::string WriteProperty(const std::string& key, const std::string& value);
    operator bool() const;
    bool operator!() const;
private:
    LoginSession* m_Session = nullptr;
};

struct LoginApi
{
    HttpResponse operator()(const Request& request);
private:
    bool ValidateCredentials(const std::string& username, const std::string& password);
};