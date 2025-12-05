#include "TimedEvent.hpp"

#include <iostream>

namespace Timed
{
    std::thread TimedEvent::CleanupThread;
    std::mutex TimedEvent::Mutex;
    std::condition_variable TimedEvent::ConditionVariable;
    std::set<TimedEvent*> TimedEvent::Events;

    static std::atomic<bool> Initialized;

    TimedEvent::TimedEvent(ExpirationDate expitationDate, decltype(m_Callback) callback) :
        m_ExpirationDate(expitationDate), m_Callback(callback)
    {
        bool expected = false;

        if (Initialized.compare_exchange_strong(expected, true) == true)
        {
            std::cout << "Initializing the TimedEvent cleanup thread\n";

            CleanupThread = std::thread(TimedEvent::CleanupThreadRoutine);
            CleanupThread.detach();
        }
    }

    TimedEvent::~TimedEvent()
    {
        std::lock_guard lock(Mutex);

        auto it = Events.find(this);
        if (it != Events.end())
        {
            Events.erase(it);
        }
    }

    void TimedEvent::AddEvent()
    {
        std::unique_lock lock(Mutex);

        Events.insert(this);
        lock.unlock();

        /* Wake the cleanup thread in case this event is now the first one to
           expire. */
        ConditionVariable.notify_all();
    }

    bool TimedEvent::Cancel()
    {
        /* Busy block until the callback returns */
        while (this->m_CallbackRunning)
        {
            std::this_thread::yield();
        }

        std::lock_guard lock(Mutex);

        auto it = Events.find(this);
        if (it == Events.end())
        {
            return false;
        }

        Events.erase(it);
        return true;
    }

    TimedEvent::ExpirationDate TimedEvent::GetNextTimeout()
    {
        /* If there are no events to check, wait one minute. */
        if (Events.size() == 0)
        {
            return std::chrono::file_clock::now() + std::chrono::minutes(1);
        }

        return (*Events.begin())->m_ExpirationDate;
    }

    void TimedEvent::CleanupThreadRoutine()
    {
        while (true)
        {
            std::unique_lock lock(Mutex);

            /* Wait until the next timeout expires, or until a new event is
               added to the queue. */
            ConditionVariable.wait_until(lock,
                                         GetNextTimeout());

            if (!(*Events.begin())->IsExpired())
            {
                continue;
            }

            auto& event = *Events.begin();
            Events.erase(Events.begin());

            event->m_CallbackRunning = true;
            lock.unlock();
            
            event->m_Callback();
            
            lock.lock();
            event->m_CallbackRunning = false;
        }
    }
}