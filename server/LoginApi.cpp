#include "LoginApi.hpp"

#include <random>
#include <mutex>

#include "TimedEvent.hpp"
#include "ErrorPage.hpp"

struct LoginSession
{
    Timed::TimedEvent m_Event;
    std::string m_SessionId;

    std::map<std::string, std::string> m_SessionData;

    static LoginSession* CreateSession();
    void CloseSession();
    void ProlongSession();
    void ProlongSession(std::unique_lock<std::mutex>& guard);
private:
    LoginSession();
};

static std::map<std::string, std::unique_ptr<LoginSession>> Sessions;

static std::mutex Mutex;
static thread_local std::mt19937 Engine = std::mt19937(std::random_device{}());
static const std::string SessionIdAlphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static std::uniform_int_distribution<size_t> SessionIdGenerator(0, SessionIdAlphabet.size() - 1);

constexpr size_t SessionIdLength = 64;

static std::string GenerateRandomSessionId()
{
    std::string result = "";

    for (size_t i = 0; i < SessionIdLength; i++)
    {
        result += SessionIdAlphabet[SessionIdGenerator(Engine)];
    }

    return result;
}

static std::string GenerateUniqueSessionId()
{
    std::string result;

    do
    {
        result = GenerateRandomSessionId();
    }
    while (Sessions.contains(result));

    return result;
}

HttpResponse LoginApi::operator()(const Request& request)
{
    std::string password;
    std::string username;

    try
    {
        auto params = ParseQueryStringUnique(request.m_Body.data());

        if (params.count("password") == 0 ||
            params.count("username") == 0)
        {
            return ErrorPage(400)(request);
        }

        username = params.at("username");
        password = params.at("password");
    }
    catch (const DuplicateQueryKeyException& e)
    {
        return ErrorPage(400)(request);
    }

    if (!ValidateCredentials(username, password))
    {
        return ErrorPage(401)(request);
    }

    LoginSession* session = LoginSession::CreateSession();
    session->m_SessionData["username"] = username;

    auto response = HttpResponse("Redirecting", 302);
    response.m_Headers["Location"] = "/";
    response.m_Headers["Set-Cookie"] = "sessionId=" + session->m_SessionId + "; Max-Age=3600; SameSite=Strict";
    return response;
}

bool LoginApi::ValidateCredentials(const std::string& username, const std::string& password)
{
    return true;
}

LoginSession::LoginSession() :
    m_Event(std::chrono::file_clock::now() + std::chrono::hours(1),
        [session = this]()
        {
            std::lock_guard guard(Mutex);

            Sessions.erase(session->m_SessionId);
        }),
    m_SessionId(GenerateUniqueSessionId())
{
    m_Event.AddEvent();
}

LoginSession* LoginSession::CreateSession()
{
    std::lock_guard guard(Mutex);
    auto sessionPtr = std::unique_ptr<LoginSession>(new LoginSession());
    auto session = sessionPtr.get();

    Sessions.insert(std::make_pair(sessionPtr->m_SessionId, std::move(sessionPtr)));
    return session;
}

void LoginSession::CloseSession()
{
    std::lock_guard guard(Mutex);

    m_Event.Cancel();
    Sessions.erase(this->m_SessionId);
}

void LoginSession::ProlongSession(std::unique_lock<std::mutex>& guard)
{
    m_Event.Cancel();
    m_Event.m_ExpirationDate = std::chrono::file_clock::now() + std::chrono::hours(1);
    m_Event.AddEvent();
}

void LoginSession::ProlongSession()
{
    std::unique_lock guard(Mutex);

    ProlongSession(guard);
}

SessionHandle::SessionHandle(const std::string& sessionId)
{
    std::unique_lock guard(Mutex);

    auto it = Sessions.find(sessionId);
    if (it == Sessions.end())
    {
        this->m_Session = nullptr;
        return;
    }

    this->m_Session = it->second.get();
    this->m_Session->ProlongSession(guard);
}

std::string SessionHandle::ReadProperty(const std::string& key)
{
    std::lock_guard guard(Mutex);

    return m_Session->m_SessionData[key];
}

std::string SessionHandle::WriteProperty(const std::string& key, const std::string& value)
{
    std::lock_guard guard(Mutex);

    std::string old = m_Session->m_SessionData[key];
    m_Session->m_SessionData[key] = value;
    return old;
}

SessionHandle::operator bool() const
{ 
    return this->m_Session != nullptr;
}

bool SessionHandle::operator!() const
{
    return this->m_Session == nullptr;
}
