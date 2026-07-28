#include "script_host.hpp"
#include <ms/core/util.hpp>
#include "io/log.hpp"
#include "script_qjs.hpp"
#include "core/alloc_ext.hpp"

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

/* JavaScript runner script list */
SCRIPT_RESOLVE_EMBED(js_runner_js)

namespace ms {
namespace browser {

    thread_local JSContext* JSTempVal::s_context = nullptr;

    static void* script_malloc(void*, size_t size) {
        return MS_ALLOC_EXT_FUNC(malloc) (size);
    }

    static void* script_calloc(void*, size_t nmemb, size_t size) {
        return MS_ALLOC_EXT_FUNC(calloc) (nmemb, size);
    }

    static void* script_realloc(void*, void* ptr, size_t size) {
        return MS_ALLOC_EXT_FUNC(realloc) (ptr, size);
    }

    static size_t script_msize(const void* ptr) {
        return MS_ALLOC_EXT_FUNC(malloc_usable_size) (ptr);
    }

    static void script_free(void*, void* ptr) {
        MS_ALLOC_EXT_FUNC(free) (ptr);
    }
    
    void ScriptHost::createEngine() {
        destroyEngine();
        auto* js = m_engine = new ScriptEngine;

        JSMallocFunctions mf;
        mf.js_free = script_free;
        mf.js_malloc = script_malloc;
        mf.js_calloc = script_calloc;
        mf.js_realloc = script_realloc;
        mf.js_malloc_usable_size = script_msize;
        
        js->runtime = JS_NewRuntime2(&mf, nullptr);
        if (! js->runtime) {
            throw ScriptEngineException("QuickJS: Cannot create runtime!");
        }
        js->context = JS_NewContext(js->runtime); // Assume only one frame for microstudio
        if (! js->context) {
            throw ScriptEngineException("QuickJS: Cannot create context for runtime!");
        }
#if defined(NDEBUG) && !defined(QUICKJS_NG)
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

    static void throwNativeException(JSContext* ctx, std::string const& path) {
        JSValue except = JS_GetException(ctx);
        char const* exMsg = JS_ToCString(ctx, except);
        JSValue stackVal = JS_GetPropertyStr(ctx, except, "stack");
        char const* exTrace = nullptr;
        if (!JS_IsUndefined(stackVal)) {
            exTrace = JS_ToCString(ctx, stackVal);
            JS_FreeValue(ctx, stackVal);
        }
        std::string err = path + " eval failed: " + exMsg + (exTrace? exTrace : " (no backtrace)");
        JS_FreeCString(ctx, exMsg);
        if (exTrace)
            JS_FreeCString(ctx, exTrace);
        JS_FreeValue(ctx, except);
        throw ScriptException(err.c_str());
    }

    std::string ScriptHost::evalScript(std::string const& script, std::string const& path) {
        JSContext* ctx = m_engine->context;
        const JSTempVal result = JS_Eval(ctx, script.c_str(), script.length(), path.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(result)) {
            throwNativeException(ctx, path);
        }
        // result js string to std string
        char const* str = JS_ToCString(m_engine->context, result);
        std::string resultStr(str);
        JS_FreeCString(m_engine->context, str);
        return resultStr;
    }

    void ScriptHost::flushJobs() {
        JSContext* outContext;
        int result = 0;
        do {
            result = JS_ExecutePendingJob(m_engine->runtime, &outContext);
        } while (result > 0);
        if (result < 0)
            throwNativeException(m_engine->context, "async");
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

    #define EVAL_STATIC_SCRIPT(global_name) evalScript(std::string(embed::__script_##global_name##_js, embed::__script_##global_name##_js_size), #global_name ".js")

    void ScriptHost::installRuntime(res::Project::Language lang) {
        try {
            EVAL_STATIC_SCRIPT(play); // play.js is required for all languages
            switch (lang) {
                case res::Project::L_MicroscriptV2:
                    EVAL_STATIC_SCRIPT(compiler);
                    EVAL_STATIC_SCRIPT(parser);
                    EVAL_STATIC_SCRIPT(processor);
                    EVAL_STATIC_SCRIPT(program);
                    EVAL_STATIC_SCRIPT(routine);
                    EVAL_STATIC_SCRIPT(runner);
                    EVAL_STATIC_SCRIPT(token);
                    EVAL_STATIC_SCRIPT(tokenizer);
                    EVAL_STATIC_SCRIPT(transpiler);
                    break;
                case res::Project::L_JavaScript:
                    EVAL_STATIC_SCRIPT(js_runner);
                    break;
                default:
                    LOG_MSGF("Script language with ID %d is not known.\n", lang);
            }
        } catch (ScriptEngineException const& see) {
            LOG_MSGF("[SCRIPT ENGINE] %s\n", see.what());
        } catch (ScriptException const& se) {
            LOG_MSGF("[SCRIPT] %s\n", se.what());
        }
    }

    void ScriptHost::onLoaded() {
        patchRuntime();
        try {
            EVAL_STATIC_SCRIPT(patch);
        } catch (ScriptEngineException const& see) {
            LOG_MSGF("[SCRIPT ENGINE] %s\n", see.what());
        } catch (ScriptException const& se) {
            LOG_MSGF("[SCRIPT] %s\n", se.what());
        }
    }

}
}
