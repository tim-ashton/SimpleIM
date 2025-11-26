#include "EventHandler.h"
#include <iostream>

namespace im_gui {

EventSubscription EventHandler::Subscribe(Event event_type, EventNotification callback) {
    const std::lock_guard lock{m_subscriptions_mutex};
    auto subscription = std::make_shared<EventNotification>(std::move(callback));
    m_subscriptions.emplace(event_type, subscription);
    return subscription;
}

void EventHandler::NotifySubscribers(Event event_type, const std::any& data) {
    const std::lock_guard lock{m_events_mutex};
    m_events.emplace_back(event_type, data);
}

void EventHandler::ProcessEvents() {
    while (const auto event{GetNextEvent()}) {
        const auto& [event_type, data] = *event;
        const std::lock_guard lock{m_subscriptions_mutex};
        const auto range{m_subscriptions.equal_range(event_type)};
        for (auto it{range.first}; it != range.second;) {
            if (auto subscription{it->second.lock()}) {
                try {
                    (*subscription)(event_type, data);
                } catch (const std::exception& e) {
                    std::cerr << "Error in event handler: " << e.what() << std::endl;
                }
                ++it;
            } else {
                it = m_subscriptions.erase(it);
            }
        }
    }
}

std::optional<std::pair<Event, std::any>> EventHandler::GetNextEvent() {
    const std::lock_guard lock{m_events_mutex};
    if (m_events.empty()) {
        return {};
    }
    auto event{m_events.front()};
    m_events.erase(m_events.begin());
    return event;
}

} // namespace im_gui