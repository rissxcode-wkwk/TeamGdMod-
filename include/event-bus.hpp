#pragma once
#ifndef ECLIPSE_EVENT_BUS_HPP
#define ECLIPSE_EVENT_BUS_HPP

#include <functional>
#include <utility>
#include <vector>

// Geode v5/v6 mengganti total sistem Event (geode::Event, EventListener, EventFilter dihapus).
// Event milik Eclipse memakai bus sederhana sendiri supaya tidak bergantung pada API Event Geode.
namespace eclipse::events {

    namespace detail {
        template <class E>
        struct Bus {
            static std::vector<std::function<void(E*)>>& listeners() {
                static std::vector<std::function<void(E*)>> list;
                return list;
            }
        };
    }

    /// @brief Base class for Eclipse events. Call post() to dispatch the event to registered listeners.
    template <class Derived>
    class PostableEvent {
    public:
        void post() {
            // iterate by index so listeners can safely register new listeners while handling
            auto& list = detail::Bus<Derived>::listeners();
            for (size_t i = 0; i < list.size(); ++i) list[i](static_cast<Derived*>(this));
        }
    };

    /// @brief Register a listener for an event type.
    template <class E, class F>
    void listen(F&& callback) {
        detail::Bus<E>::listeners().emplace_back(std::forward<F>(callback));
    }

}

#endif // ECLIPSE_EVENT_BUS_HPP
