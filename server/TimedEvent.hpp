#pragma once

#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <condition_variable>
#include <set>

namespace Timed
{
    struct TimedEvent
    {
        using ExpirationDate = std::chrono::time_point<std::chrono::file_clock>;
        
        ExpirationDate m_ExpirationDate;
        std::function<void(void)> m_Callback;
        std::atomic<bool> m_CallbackRunning = false;

        static std::mutex Mutex;
        static std::condition_variable ConditionVariable;
        static std::set<TimedEvent*> Events;

        TimedEvent(ExpirationDate expitationDate, decltype(m_Callback) callback);

        TimedEvent(TimedEvent&& other) = default;

        ~TimedEvent();

        bool IsExpired() const
        {
            return std::chrono::file_clock::now() > m_ExpirationDate;
        }

        bool operator<(const TimedEvent& other) const
        {
            return this->m_ExpirationDate < other.m_ExpirationDate;
        }

        void AddEvent();

        bool Cancel();
    private:
        static std::thread CleanupThread;

        static ExpirationDate GetNextTimeout();

        static void CleanupThreadRoutine();
    };
}