#include "platform/platform.hpp"
#include "log.hpp"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifdef PLATFORM_NT
    #include <io.h>
    #ifndef write
    #define write _write
    #endif
    #ifndef fileno
    #define fileno _fileno
    #endif
#endif
#ifdef PLATFORM_UNIX
    #include <unistd.h>
#endif

namespace ms {
namespace io {

    void LogDispatcher::addStandardOutput() {
        addOutput([](char const* message, size_t sz){
            write(fileno(stdout), message, sz);
        });
    }

    void LogDispatcher::logf(char const* fmt, ...) {
        static thread_local char messageBuffer[1 << 12];

        va_list va;
        va_start(va, fmt);
        vsnprintf(messageBuffer, sizeof(messageBuffer), fmt, va);
        va_end(va);

        log(messageBuffer);
    }

    void LogDispatcher::log(char const* message) {
        size_t sz = strlen(message);
        for (auto itr = m_loggers.cbegin(); itr != m_loggers.cend(); ++itr)
            (*itr)(message, sz);
    }

}
}
