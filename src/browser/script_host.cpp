#include "script_host.hpp"
#include "core/util.hpp"
#include "io/log.hpp"
#include "script_qjs.hpp"

/* Script list */
SCRIPT_RESOLVE_EMBED(play_js)
SCRIPT_RESOLVE_EMBED(patch_js)

/* Microscript V2 interpreter script list */
SCRIPT_RESOLVE_EMBED(compiler_js)
SCRIPT_RESOLVE_EMBED(parser_js)
SCRIPT_RESOLVE_EMBED(processor_js)
SCRIPT_RESOLVE_EMBED(program_js)
SCRIPT_RESOLVE_EMBED(routine_js)
SCRIPT_RESOLVE_EMBED(runner_js)
SCRIPT_RESOLVE_EMBED(token_js)
SCRIPT_RESOLVE_EMBED(tokenizer_js)
SCRIPT_RESOLVE_EMBED(transpiler_js)

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
        JSContext* ctx = m_engine->context;
        const JSTempVal result = JS_Eval(ctx, script.c_str(), script.length(), path.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(result)) {
            JSValue except = JS_GetException(ctx);
            char const* exMsg = JS_ToCString(ctx, except);
            JSValue stackVal = JS_GetPropertyStr(ctx, except, "stack");
            char const* exTrace = "";
            if (!JS_IsUndefined(stackVal)) {
                exTrace = JS_ToCString(ctx, stackVal);
                JS_FreeValue(ctx, stackVal);
            }
            std::string err = path + " eval failed: " + exMsg + exTrace;
            JS_FreeCString(ctx, exMsg);
            JS_FreeCString(ctx, exTrace);
            JS_FreeValue(ctx, except);
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
        } catch (ScriptEngineException const& see) {
            LOG_MSGF("[SCRIPT ENGINE] %s\n", see.what());
        } catch (ScriptException const& se) {
            LOG_MSGF("[SCRIPT] %s\n", se.what());
        }
    }

    #define EVAL_STATIC_SCRIPT(global_name) evalScript(std::string(embed::__script_##global_name, embed::__script_##global_name##_size), #global_name)

    void ScriptHost::installRuntime() {
        try {
            EVAL_STATIC_SCRIPT(play_js);
            EVAL_STATIC_SCRIPT(compiler_js);
            EVAL_STATIC_SCRIPT(parser_js);
            EVAL_STATIC_SCRIPT(processor_js);
            EVAL_STATIC_SCRIPT(program_js);
            EVAL_STATIC_SCRIPT(routine_js);
            EVAL_STATIC_SCRIPT(runner_js);
            EVAL_STATIC_SCRIPT(token_js);
            EVAL_STATIC_SCRIPT(tokenizer_js);
            EVAL_STATIC_SCRIPT(transpiler_js);
        } catch (ScriptEngineException const& see) {
            LOG_MSGF("[SCRIPT ENGINE] %s\n", see.what());
        } catch (ScriptException const& se) {
            LOG_MSGF("[SCRIPT] %s\n", se.what());
        }
    }

    void ScriptHost::onLoaded() {
        patchRuntime();
        try {
            EVAL_STATIC_SCRIPT(patch_js);
        } catch (ScriptEngineException const& see) {
            LOG_MSGF("[SCRIPT ENGINE] %s\n", see.what());
        } catch (ScriptException const& se) {
            LOG_MSGF("[SCRIPT] %s\n", se.what());
        }
    }

}
}
