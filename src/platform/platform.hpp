#pragma once

/*
 * About platform/abstracted:
 * 
 * Abstracted platform classes have priority over OS-specific implementations
 * since they are designed to be already cross-platform
 * (when platform is managed at library level, think Raylib or SDL)
 */

#define PLATFORM_ABSTRACTED
#if defined(__EMSCRIPTEN__)
    #define PLATFORM_BROWSER
#elif defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_NT
    #ifdef _MSC_VER
        #define PLATFORM_NT_MSVC _MSC_VER
    #endif
#elif defined(__unix__) || defined(__unix)
    #define PLATFORM_UNIX
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_DARWIN
#else
    // TODO: Error???
    #define PLATFORM_UNKNOWN
#endif
