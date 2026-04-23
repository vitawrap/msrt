#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <vector>

namespace ms {

    class EventQueue;

    /**
     * @brief Event wildcard to allow invocation from base class to specialized event
     */
    class EventAny {
    public:
        template <typename... Args>
        inline void invoke(Args&&... args);

        template <typename... Args>
        inline void invokeDeferred(Args&&... args);

        template <typename... Args>
        inline void invokeDeferredQueue(Args&&... args, EventQueue* queue);

        template <typename... Args>
        inline size_t connect(std::function<void(Args...)> const& func, int connFlags = 0);

        virtual bool disconnect(size_t id) = 0;
        virtual size_t connections() const = 0;
    };

    /**
     * @brief Event specialization used in classes
     */
    template <typename... Args>
    class Event : private EventAny {
    public:
        typedef std::function<void(Args...)> FuncType;
        enum ConnectionFlags {
            CONN_DEAD = 1,  // should only be set by event queue!
            CONN_ONCE = 2   // flag connection as disconnected once invoked.
        };
        struct Connection {
            FuncType function;
            size_t id;
            int flags;
        };
    protected:
        static inline uint64_t s_idgen = 0; // atomic because of mutex

        mutable std::recursive_mutex m_mutex;
        std::vector<struct Connection> m_slots;
        uint64_t m_invokeDepth;
        bool m_hasZombies;

        void sweepDeadEntries() {
            if (m_hasZombies) {
                for (auto itr = m_slots.cbegin(); itr != m_slots.cend();) {
                    if ((*itr).flags & CONN_DEAD)
                        itr = m_slots.erase(itr);
                    else
                        ++itr;
                }
                m_hasZombies = false;
            }
        }

    public:
        Event():
            m_invokeDepth(0ul)
        {}

        Connection connect(FuncType slot, int connFlags = 0) {
            Connection conn;
            conn.flags = connFlags;
            conn.function = slot;
            {
                std::lock_guard<decltype(m_mutex)> lock(m_mutex);
                conn.id = ++s_idgen;
                m_slots.push_back(conn);
            }
            return conn;
        }

        bool disconnect(size_t id) override {
            std::lock_guard<decltype(m_mutex)> lock(m_mutex);
            /** TODO: Look at std::remove_if */
            for (auto itr = m_slots.begin(); itr != m_slots.end(); ++itr) {
                Connection& conn = *itr;
                if (conn.id == id) {
                    if (m_invokeDepth > 0) {
                        conn.flags |= CONN_DEAD;
                        conn.function = nullptr; // unref function
                        m_hasZombies = true;
                    } else
                        m_slots.erase(itr);
                    return true;
                }
            }
            return false;
        }

        size_t connections() const override {
            std::lock_guard<decltype(m_mutex)> lock(m_mutex);
            return m_slots.size();
        }

        void invoke(Args... args) {
            std::lock_guard<decltype(m_mutex)> lock(m_mutex);
            ++m_invokeDepth;

            for (auto& slot : m_slots) {
                if (!(slot.flags & CONN_DEAD)) {
                    slot.function(args...);
                    if (slot.flags & CONN_ONCE)
                        slot.flags |= CONN_DEAD;
                }
            }

            --m_invokeDepth;
            if (m_invokeDepth == 0)
                sweepDeadEntries();
        }

        inline void invokeDeferredQueue(Args... args, EventQueue* queue);

        inline void invokeDeferred(Args... args);

        operator EventAny*() { return this; }
    };

    template <typename... Args>
    void EventAny::invoke(Args&&... args) {
        dynamic_cast<Event<Args...>*>(this)->invoke(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void EventAny::invokeDeferred(Args&&... args) {
        dynamic_cast<Event<Args...>*>(this)->invokeDeferred(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void EventAny::invokeDeferredQueue(Args&&... args, EventQueue* queue) {
        dynamic_cast<Event<Args...>*>(this)->invokeDeferredQueue(std::forward<Args>(args)..., queue);
    }

    template <typename... Args>
    size_t EventAny::connect(std::function<void(Args...)> const& func, int connFlags) {
        auto conn = dynamic_cast<Event<Args...>*>(this)->connect(func, connFlags);
        return conn.id;
    }

    /** This type alias only exists to be able to reference enums */
    using EventEnum = Event<>;

    class EventQueue {
    public:
        typedef std::function<void()> EventFuncType;
    protected:
        static EventQueue* s_mainQueue;
        mutable std::recursive_mutex m_mutex;
        std::queue<EventFuncType> m_notifications;
    public:
        void flushNotifications() {
            decltype(m_notifications) saved;
            {
                std::lock_guard<decltype(m_mutex)> lock(m_mutex);
                if (m_notifications.empty())
                    return;
                std::swap(saved, m_notifications);
            }
            while (saved.size()) {
                saved.front()();
                saved.pop();
            }
        }

        void queueNotification(EventFuncType func) {
            std::lock_guard<decltype(m_mutex)> lock(m_mutex);
            m_notifications.push(func);
        }

        static EventQueue* get() {
            if (!s_mainQueue)
                s_mainQueue = new EventQueue;
            return s_mainQueue;
        }
    };

    template <typename... Args>
    void Event<Args...>::invokeDeferredQueue(Args... args, EventQueue* queue) {
        queue->queueNotification([this, args...](){
            /** TODO: what if invoker gets freed before notification dispatch?? */
            this->invoke(args...);
        });
    }

    template <typename... Args>
    void Event<Args...>::invokeDeferred(Args... args) {
        invokeDeferredQueue(args..., EventQueue::get()); // invoke in main thread queue
    }

}
