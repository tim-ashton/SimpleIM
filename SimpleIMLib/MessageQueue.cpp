#include "MessageQueue.h"

MessageQueue::MessageQueue()
    : m_terminate(false)
{
}

MessageQueue::~MessageQueue()
{
    wakeUp();
}

void MessageQueue::push(const Message& message)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_terminate) {
            return; // Don't add messages if terminating
        }
        m_queue.push(message);
    }
    m_condition.notify_one();
}

Message MessageQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return !m_queue.empty() || m_terminate; });
    
    if (m_terminate && m_queue.empty()) {
        // Return empty message if terminating and no messages left
        return Message(static_cast<MessageType>(0), "");
    }
    
    Message message = m_queue.front();
    m_queue.pop();
    return message;
}

bool MessageQueue::tryPop(Message& message, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (m_condition.wait_for(lock, timeout, [this] { return !m_queue.empty() || m_terminate; })) {
        if (!m_queue.empty()) {
            message = m_queue.front();
            m_queue.pop();
            return true;
        }
    }
    
    return false;
}

bool MessageQueue::empty() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

size_t MessageQueue::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void MessageQueue::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<Message> empty;
    m_queue.swap(empty);
}

void MessageQueue::wakeUp()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_terminate = true;
    }
    m_condition.notify_all();
}
