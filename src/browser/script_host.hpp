#pragma once

#include <stdexcept>
#include "resources/project.hpp"

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
         * Event loop for the script engine
         */
        void flushJobs();

        /**
         * Install libraries necessary to emulate browser environment
         */
        void installLibs();

        /**
         * Install microstudio runtime onto the script engine
         */
        void installRuntime(res::Project::Language lang = res::Project::L_MicroscriptV2);

        /**
         * Replace evaluated script classes with C++ equivalents
         */
        void patchRuntime();

        /**
         * Reproduce the load event instantiating the runtime
         */
        void onLoaded();
    };

}
}
