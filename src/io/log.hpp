#pragma once

#include <ms/core/util.hpp>
#include <stddef.h>
#include <set>

namespace ms {
namespace io {

    /**
     * @brief Logging interface utility class.
     * Dispatches logged messages to all registered listeners.
     * For technical reasons this is class is a singleton.
     */
    class LogDispatcher {
    public:
        typedef void (*TFunc) (char const* message, size_t sz);

    private:
        std::set<TFunc> m_loggers;

    public:
        static LogDispatcher& get() {
            static LogDispatcher instance;
            return instance;
        }

        /**
         * @brief Add (unique) logger output to this dispatcher.
         */
        void addOutput(TFunc logFunc) {
            m_loggers.emplace(logFunc);
        }

        void addStandardOutput();

        void clearOutputs() {
            m_loggers.clear();
        }

        /**
         * @brief Print formatted message.
         * Newline is used as a flush token.
         */
        void FUNC_FA_PRINTFLIKE(2, 3) logf(char const* fmt, ...);

        /**
         * @brief Print simple message.
         * Newline is used as a flush token.
         */
        void log(char const* fmt);
    };

}
}

/* Macros */

/**
 * @brief Log formatted message with the log dispatcher.
 */
#define LOG_MSGF(...) ms::io::LogDispatcher::get().logf(__VA_ARGS__)

/**
 * @brief Log simple message with the log dispatcher.
 */
#define LOG_MSG(msg) ms::io::LogDispatcher::get().log(msg)
