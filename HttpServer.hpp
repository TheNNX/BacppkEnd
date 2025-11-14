#pragma once

#include <map>
#include <string>
#include "InetSocketWrapper.h"
#include "Http.hpp"
#include <fstream>
#include <algorithm>
#include <functional>
#include <iostream>
#include <thread>
#include <semaphore>
#include <memory>

#include "ErrorPage.hpp"

using namespace InetSocketWrapper;

struct Responder
{
    std::function<HttpResponse(const Request& request)> m_Respond;
    std::map<std::string, Responder> m_Children;

    HttpResponse operator()(const Request& request) const
    {
        return this->m_Respond(request);
    }

    HttpResponse Respond(const Request& request) const
    {
        return (*this)(request);
    }

    const Responder* GetChild(const std::string& name) const
    { 
        if (m_Children.count(name) == 0)
        {
            return nullptr;
        }

        return &m_Children.at(name);
    }

    template<typename T>
    Responder(T respond) : m_Respond(respond)
    { 
    }

    Responder() = default;
};

struct HttpService;
struct Alias
{
    std::string m_To;
    HttpService& m_Service;

    Alias(HttpService& service, const std::string& to) :
        m_To(to), m_Service(service) 
    {
    }

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

    HttpService(const std::string& interfce, uint16_t port = 80, InternetProtocol protocol = IPv4);

    HttpResponse GetResponse(const Request& request) const;
    std::thread Run();
};

struct HttpClientWorker
{
    const HttpService& m_Service;
    std::thread m_Thread;
    std::binary_semaphore m_Semaphore = std::binary_semaphore(0);

    HttpClientWorker(Connection&& connection, const HttpService& service) :
        m_Service(service), 
        m_Thread([&]()
                 {    
                     Connection moved = std::move(connection);
                     m_Semaphore.release();
                     return this->WorkerFunction(std::move(moved));
                 })
    {
        m_Semaphore.acquire();
        m_Thread.detach();
    }

    struct WorkerCounterAcquirer
    {
        WorkerCounterAcquirer()
        {
            GetNumberOfThreadsRef()++;
        }

        ~WorkerCounterAcquirer()
        {
            GetNumberOfThreadsRef()--;
        }

        static std::atomic<int>& GetNumberOfThreadsRef()
        {
            static std::atomic<int> numThreads = 0;
            return numThreads;
        }
    };

    void WorkerFunction(Connection&& connection);
};