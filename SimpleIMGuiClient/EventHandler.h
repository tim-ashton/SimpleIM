#pragma once

#include <any>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <optional>

namespace im_gui {

enum class Event {
    LOGIN_REQUESTED,
    LOGIN_SUCCESS,
    LOGIN_FAILED,
    MESSAGE_SENT,
    USER_JOINED,
    USER_LEFT,
    CONNECTION_LOST,
    CONNECTION_ESTABLISHED
};

using EventNotification = std::function<void(Event, const std::any&)>;
using EventSubscription = std::shared_ptr<EventNotification>;

class EventHandler {
public:
    EventSubscription Subscribe(Event event_type, EventNotification callback);
    void NotifySubscribers(Event event_type, const std::any& data = {});
    void ProcessEvents();

private:
    std::mutex m_subscriptions_mutex;
    std::mutex m_events_mutex;
    
    std::unordered_multimap<Event, std::weak_ptr<EventNotification>> m_subscriptions;
    std::vector<std::pair<Event, std::any>> m_events;
    
    // Helper function to safely get next event from queue
    std::optional<std::pair<Event, std::any>> GetNextEvent();
};

} // namespace im_gui