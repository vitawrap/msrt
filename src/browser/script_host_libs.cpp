#include "core/app.hpp"
#include "script_host.hpp"
#include "script_qjs.hpp"
#include "io/log.hpp"

#include "core/events.hpp"

/** quickjs really likes using mixed designators for JSCFunctionListEntry and it creates a flood of warnings... */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-designator"

/**
 * All of those methods assume JSTempVal scoping was set up.
 */
namespace ms {
namespace browser {

    /* Util */

    static inline JSValue getClassProtoOrThrow(JSContext* ctx, JSValueConst const& new_target, JSClassID clid, char const* clname) {
        JSValue proto;
        if (JS_IsUndefined(new_target)) {
            proto = JS_GetClassProto(ctx, clid);
        } else {
            proto = JS_GetPropertyStr(ctx, new_target, "prototype");
            if (JS_IsException(proto)) {
                std::string exMsg = std::string(clname) + ": Invalid prototype when constructing instance!";
                throw ScriptEngineException(exMsg.c_str());
            }
        }
        return proto; // not a temp value, but should be assigned to one when leaving this function!
    }

#pragma region Objects

    template <typename T, JSClassID const& classID>
    static JSValue constructObject(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
        JSTempVal proto = getClassProtoOrThrow(ctx, new_target, classID, typeid(T).name());
        JSValue obj = JS_NewObjectProtoClass(ctx, proto, classID);
        if (JS_IsException(obj)) {
            std::string except = typeid(T).name();
            throw ScriptEngineException((except + ": Failed to construct new C instance!").c_str());
        }
        auto* proxy = new ScriptProxy<T>(ctx);
        JS_SetOpaque(obj, proxy);
        //proxy->setValue(JS_DupValue(ctx, obj)); // dup into opaque: don't hold onto temp value container
        proxy->setValue(obj);
        return obj;
    }

    template <typename T>
    static void destructObject(JSRuntime* rt, JSValue self) {
        auto* proxy = reinterpret_cast<ScriptProxy<T>*>(JS_GetOpaque(self, JS_GetClassID(self)));
        DEBUG_ASSERT(proxy && "wrong classID in destructObject!");
        delete proxy; // also destroys encapsulated T. Every qjs subclass MUST derive this destructor, since no dynamic binding test will be made.
    }

    template <typename T>
    static inline T* opaqueToObject(JSValueConst const& v) {
        void* opaque = JS_GetOpaque(v, JS_GetClassID(v));
        if (!opaque)
            throw ScriptEngineException("Wrong ClassID in opaqueToObject!");
        return ScriptProxy<T>::cast(opaque);
    }

    static bool inheritPrototype(JSContext* ctx, JSValueConst const& derivedProto, char const* base) {
        JSTempVal global = JS_GetGlobalObject(ctx);
        JSTempVal ctor = JS_GetPropertyStr(ctx, global, base);
        if (JS_IsException(ctor)) return false;
        JSTempVal proto = JS_GetPropertyStr(ctx, ctor, "prototype"); // not GetPrototype because that'd be "Function"
        if (JS_IsException(ctor)) return false;
        return JS_SetPrototype(ctx, derivedProto, proto) != -1;
    }

#pragma endregion

    static void installConsole(JSContext* ctx) {
        JSTempVal globalThis = JS_GetGlobalObject(ctx);
        JSValue console = JS_NewObject(ctx);
        JSValue log = JS_NewCFunction(ctx,
        [](JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) -> JSValue {
            for (int i = 0; i < argc; ++i) {
                char const* str = JS_ToCString(ctx, argv[i]);
                LOG_MSGF("%s ", str);
                JS_FreeCString(ctx, str);
            }
            LOG_MSG("\n");
            return JS_UNDEFINED;
        }, "log", 1);
        JS_SetPropertyStr(ctx, console, "log", log);
        JS_SetPropertyStr(ctx, globalThis, "console", console);
    }

    /**
     * Minimum viable product for HTML interop from play.js
     * Exposes a window and console,
     * Supports elements "a", "canvas", "canvaswrapper", and "script"
     */
    void ScriptHost::installLibHTML() {
        auto* ctx = m_engine->context;
        JSValue globalThis = JS_GetGlobalObject(ctx);

        // requestAnimationFrame
        JSValue reqFrame = JS_NewCFunction(ctx, 
        [](JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) -> JSValue {
            if (argc && JS_IsFunction(ctx, argv[0])) {
                JSValue ownFn = JS_DupValue(ctx, argv[0]);
                auto browser = Application::get()->getBrowserContext();
                browser->addRepaintListener([ctx, ownFn](){
                    JSValue tstamp = JS_NewObject(ctx); /** TODO: DOMHighResTimeStamp!!! */
                    JS_Call(ctx, ownFn, JS_UNDEFINED, 1, &tstamp);
                    JS_FreeValue(ctx, ownFn);
                    JS_FreeValue(ctx, tstamp);
                });
                return JS_UNDEFINED;
            }
            JS_ThrowTypeError(ctx, "%s arg in requestAnimationFrame", argc? "not a Function" : "expected 1");
            return JS_EXCEPTION; /** TODO: this should be a counter system? */
        }, "requestAnimationFrame", 1);
        JS_SetPropertyStr(ctx, globalThis, "requestAnimationFrame", reqFrame);

        // install conventional objects
        installConsole(ctx);

        // re-add globalThis as window
        JS_SetPropertyStr(ctx, globalThis, "window", globalThis);
    }

}
}

#pragma clang diagnostic pop
