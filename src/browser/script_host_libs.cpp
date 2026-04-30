#include "core/app.hpp"
#include "script_host.hpp"
#include "script_qjs.hpp"
#include "io/log.hpp"

#include "core/events.hpp"
#include "classes/screen.hpp"
#include "classes/player.hpp"

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

    static JSValue Proto_notImplemetedQuiet(JSContext* ctx, JSValueConst self, int argc, JSValueConst *argv) {
        return JS_UNDEFINED;
    }

    static JSValue Proto_notImplemeted(JSContext* ctx, JSValueConst self, int argc, JSValueConst *argv) {
        JS_ThrowInternalError(ctx, "%s", "Not implemented!");
        return JS_EXCEPTION;
    }

#pragma endregion
#pragma region Runtime*

    static JSClassID classId_Runtime;

    static JSCFunctionListEntry defineRuntime[] = {
    };

    static void installRuntime(JSContext* ctx) {
        JSClassDef cdef{ .class_name = "Runtime", .finalizer = &destructObject<Runtime>, .gc_mark = nullptr };
        JS_NewClassID(&classId_Runtime);
        JS_NewClass(JS_GetRuntime(ctx), classId_Runtime, &cdef);
        JSValue proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, proto, defineRuntime, countof(defineRuntime));
        // global constructor
        JSValue ctor = JS_NewCFunction2(ctx, &constructObject<Runtime, classId_Runtime>,
            cdef.class_name, 1, JS_CFUNC_constructor, 0);
        JS_SetConstructor(ctx, ctor, proto);
        JS_SetClassProto(ctx, classId_Runtime, proto);
        JSTempVal globalThis = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, globalThis, cdef.class_name, ctor);
    }

#pragma endregion
#pragma region Screen

    static JSClassID classId_Screen;

    static JSCFunctionListEntry defineScreen[] = {
        JS_CFUNC_DEF("initContext", 0, Proto_notImplemetedQuiet),
        JS_CFUNC_DEF("getInterface", 0, Proto_notImplemetedQuiet),
        JS_CFUNC_DEF("updateInterface", 0, Proto_notImplemeted),
        JS_CFUNC_DEF("clear", 1, Proto_notImplemeted),
        JS_CFUNC_DEF("initDraw", 0, Proto_notImplemeted),
        JS_CFUNC_DEF("setColor", 1, Proto_notImplemeted),
    };

    static JSValue constructScreen(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
        JSValue val = constructObject<Screen, classId_Screen>(ctx, new_target, argc, argv);
        JSValueConst& rtVal = argv[0];
        if (argc && JS_GetClassID(rtVal) == classId_Runtime) {
            Runtime* rt = opaqueToObject<Runtime>(rtVal);
            if (rt) JS_DupValue(ctx, rtVal);
            opaqueToObject<Screen>(val)->setRuntime(rt);
        } else {
            JS_FreeValue(ctx, val);
            JS_ThrowInternalError(ctx, "%s", "Trying to construct Screen without Runtime!");
            return JS_EXCEPTION;
        }
        return val;
    }

    static void gcMarkScreen(JSRuntime* rt, JSValueConst self, JS_MarkFunc markFunc) {
        auto* screen = opaqueToObject<Screen>(self);
        if (screen->getRuntime())
            JS_MarkValue(rt, ScriptProxy<Runtime>::toValue(screen->getRuntime()), markFunc);
    }

    static void destructScreen(JSRuntime* rt, JSValueConst self) {
        auto* screen = opaqueToObject<Screen>(self);
        if (screen->getRuntime()) {
            JS_FreeValueRT(rt, ScriptProxy<Runtime>::toValue(screen->getRuntime()));
            screen->setRuntime(nullptr);
        }
        destructObject<Screen>(rt, self);
    }

    static void installScreen(JSContext* ctx) {
        JSClassDef cdef{ .class_name = "Screen", .finalizer = destructScreen, .gc_mark = gcMarkScreen };
        JS_NewClassID(&classId_Screen);
        JS_NewClass(JS_GetRuntime(ctx), classId_Screen, &cdef);
        JSValue proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, proto, defineScreen, countof(defineScreen));
        // global constructor
        JSValue ctor = JS_NewCFunction2(ctx, constructScreen, cdef.class_name, 1, JS_CFUNC_constructor, 0);
        JS_SetConstructor(ctx, ctor, proto);
        JS_SetClassProto(ctx, classId_Screen, proto);
        JSTempVal globalThis = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, globalThis, cdef.class_name, ctor);
    }

#pragma endregion
#pragma region Player

    static JSClassID classId_Player;

    static void gcMarkPlayer(JSRuntime* rt, JSValueConst self, JS_MarkFunc markFunc) {
        auto* player = opaqueToObject<Player>(self);
        if (player->getRuntime())
            JS_MarkValue(rt, ScriptProxy<Runtime>::toValue(player->getRuntime()), markFunc);
    }

    static void destructPlayer(JSRuntime* rt, JSValueConst self) {
        auto* player = opaqueToObject<Player>(self);
        if (player->getRuntime()) {
            JS_FreeValueRT(rt, ScriptProxy<Runtime>::toValue(player->getRuntime()));
            player->setRuntime(nullptr);
        }
        destructObject<Runtime>(rt, self);
    }

    static JSValue PlayerProto_start(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) {
        auto* player = opaqueToObject<Player>(self);
        // construct runtime using proxy exposed to JS, so that it sees (and collects) this object as well
        auto* runtimeProxy = new ScriptProxy<Runtime>(ctx);
        JS_DupValue(ctx, runtimeProxy->getValue()); // create strong reference
        player->setRuntime(runtimeProxy->getObject());
        player->start();
        return JS_UNDEFINED;
    }

    static JSCFunctionListEntry definePlayer[] = {
        JS_CFUNC_DEF("start", 0, PlayerProto_start),
    };

    static void installPlayer(JSContext* ctx) {
        JSClassDef cdef{ .class_name = "Player", .finalizer = destructPlayer, .gc_mark = gcMarkPlayer };
        JS_NewClassID(&classId_Player);
        JS_NewClass(JS_GetRuntime(ctx), classId_Player, &cdef);
        JSValue proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, proto, definePlayer, countof(definePlayer));
        // global constructor
        JSValue ctor = JS_NewCFunction2(ctx, &constructObject<Player, classId_Player>,
            cdef.class_name, 1, JS_CFUNC_constructor, 0);
        JS_SetConstructor(ctx, ctor, proto);
        JS_SetClassProto(ctx, classId_Player, proto);
        JSTempVal globalThis = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, globalThis, cdef.class_name, ctor);
    }

#pragma endregion

    static void installFakeObjects(JSContext* ctx) {
        JSTempVal globalThis = JS_GetGlobalObject(ctx);
        // mute "'CanvasRenderingContext2D' is not defined" error
        JSValue rc2d = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, rc2d, "prototype", JS_NewObject(ctx));
        JS_SetPropertyStr(ctx, globalThis, "CanvasRenderingContext2D", rc2d);
        // add Window.navigator
        JSValue nav = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, nav, "language", JS_NewString(ctx, "en-US"));
        JS_SetPropertyStr(ctx, globalThis, "navigator", nav);
    }

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
     * Exposes a window and console + ms objects converted to C++
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
        installFakeObjects(ctx);

        // re-add globalThis as window
        JS_SetPropertyStr(ctx, globalThis, "window", globalThis);
    }

    /**
     * After play.js is executed, this replaces all main global script classes with ours
     */
    void ScriptHost::patchRuntime() {
        auto* ctx = m_engine->context;
        browser::installRuntime(ctx);
        browser::installScreen(ctx);
        browser::installPlayer(ctx);
    }

}
}

#pragma clang diagnostic pop
