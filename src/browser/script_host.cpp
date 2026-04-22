#include "script_host.hpp"
#include "core/util.hpp"
#include "io/log.hpp"
#include "script_qjs.hpp"

/* Script list */
SCRIPT_RESOLVE_EMBED(__script_play_js)

namespace ms {
namespace browser {

    thread_local JSContext* JSTempVal::s_context = nullptr;

    void ScriptHost::createEngine() {
        destroyEngine();
        auto* js = m_engine = new ScriptEngine;

        js->runtime = JS_NewRuntime(); // TODO: Test with fast allocator libs
        if (! js->runtime) {
            throw ScriptEngineException("QuickJS: Cannot create runtime!");
        }
        js->context = JS_NewContext(js->runtime); // Assume only one frame for microstudio
        if (! js->context) {
            throw ScriptEngineException("QuickJS: Cannot create context for runtime!");
        }
#ifdef NDEBUG
        // try to have a better memory footprint in release mode
        JS_SetStripInfo(js->runtime, JS_STRIP_DEBUG | JS_STRIP_SOURCE);
#endif

        // Browser API
        JSTempVal::scopeContext(m_engine->context);
        installLibs();
    }

    void ScriptHost::destroyEngine() {
        if (m_engine) {
            JS_FreeContext(m_engine->context);
            JS_RunGC(m_engine->runtime); // gc has to be run manually before shutdown
            JS_FreeRuntime(m_engine->runtime);
            delete m_engine;
            m_engine = nullptr;
        }
    }

    std::string ScriptHost::evalScript(std::string const& script, std::string const& path) {
        const JSTempVal result = JS_Eval(m_engine->context, script.c_str(), script.length(), path.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(result)) {
            const JSTempVal except = JS_GetException(m_engine->context);
            char const* exMsg = JS_ToCString(m_engine->context, except);
            std::string err = path + " eval failed: " + exMsg;
            JS_FreeCString(m_engine->context, exMsg);
            throw ScriptException(err.c_str());
        }
        // result js string to std string
        char const* str = JS_ToCString(m_engine->context, result);
        std::string resultStr(str);
        JS_FreeCString(m_engine->context, str);
        return resultStr;
    }

    void ScriptHost::installLibs() {        
        try {
            installLibHTML();
            installLibCanvas();
        } catch (ScriptEngineException const& see) {
            LOG_MSGF("[SCRIPT ENGINE] %s\n", see.what());
        } catch (ScriptException const& se) {
            LOG_MSGF("[SCRIPT] %s\n", se.what());
        }
    }

    void ScriptHost::installRuntime() {
        try {
            evalScript(std::string(embed::__script_play_js, embed::__script_play_js_size), "play.js");
        } catch (ScriptEngineException const& see) {
            LOG_MSGF("[SCRIPT ENGINE] %s\n", see.what());
        } catch (ScriptException const& se) {
            LOG_MSGF("[SCRIPT] %s\n", se.what());
        }
    }

}
}
