#pragma once

#include <stdexcept>

#ifdef _SCRIPT_QJS_HPP_
#error "script_qjs.hpp should not be included here!"
#endif

namespace ms {
namespace browser {

    /**
     * Runtime error wrapper for script exceptions
     */
    class ScriptException : public std::runtime_error {
    public:
        ScriptException(char const* err)
            : std::runtime_error(err) {}
    };

    /**
     * Script error on the engine side (QuickJS, ...)
     */
    class ScriptEngineException : public std::runtime_error {
    public:
        ScriptEngineException(char const* err)
            : std::runtime_error(err) {}
    };
    
    class Context;
    class ScriptEngine;

    /**
     * @brief JavaScript host for browser context.
     * This is specifically tailored for QuickJS.
     */
    class ScriptHost {
        Context* m_browser;
        ScriptEngine* m_engine;

        void installLibHTML();
        void installLibCanvas();
    public:
        ScriptHost(Context* browser) :
            m_browser(browser),
            m_engine(nullptr)
        {}

        /**
         * Create the encapsulated JS engine
         */
        void createEngine();

        /**
         * Deallocate JS engine and make sure there are no ties
         * left with the environment
         */
        void destroyEngine();

        /**
         * Evaluate script contents in engine and return result as a string
         */
        std::string evalScript(std::string const& script, std::string const& path = "");

        /**
         * Install libraries necessary to emulate browser environment
         */
        void installLibs();

        /**
         * Install microstudio runtime onto the script engine
         */
        void installRuntime();
    };

}
}

/* Retrieve a script entry in script_embed.s */
#define SCRIPT_RESOLVE_EMBED(global_name) \
extern const char global_name[]; extern const unsigned global_name##_size; \
namespace embed { static const char* global_name = ::global_name; \
static const unsigned global_name##_size = ::global_name##_size; }
