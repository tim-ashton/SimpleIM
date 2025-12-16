#pragma once

#include <Message.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

class MessageQueue
{
public:
    MessageQueue();
    ~MessageQueue();
    
    void push(const Message& message);
    Message pop();
    bool tryPop(Message& message, std::chrono::milliseconds timeout = std::chrono::milliseconds(10));
    bool empty() const;
    size_t size() const;
    void clear();
    
    void wakeUp();

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<Message> m_queue;
    bool m_terminate = false;
};
