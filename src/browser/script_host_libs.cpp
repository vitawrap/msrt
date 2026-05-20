#include "core/app.hpp"
#include "script_host.hpp"
#include "script_qjs.hpp"
#include "io/log.hpp"

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
#pragma region Screen

    static JSClassID classId_Screen;

    static JSClassID getRuntimeClassID();

    static JSValue ScreenProto_simple(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv, int magic) {
        auto* screen = opaqueToObject<Screen>(self);
        switch (magic) {
            case 0: screen->startControl(); break;
            case 1: screen->initContext(); break;
            case 2: screen->initDraw(); break;
            case 3: screen->closeDrawOp(); break;
            case 4: screen->resize(); break;
        }
        MAYBE_RETHROW_EXCEPTION_V(ctx, JS_UNDEFINED);
    }

    static inline bool JSValueToScreenColor(JSContext* ctx, Screen* screen, JSValue cVal, uint32_t& color) {
        if (JS_IsString(cVal)) {
            char const* colorStr = JS_ToCString(ctx, cVal);
            color = screen->stringToColor(colorStr);
            JS_FreeCString(ctx, colorStr);
        } else if (JS_IsNumber(cVal) && (JS_ToUint32(ctx, &color, cVal) == 0)) {
            color = screen->decimalToColor(color);
        } else
            return JS_IsUndefined(cVal); // allow undefined: it means we keep the old value
        return true;
    }

    static JSValue ScreenProto_colorArg(JSContext* ctx, JSValueConst self, int argc, JSValueConst *argv, int magic) {
        auto* screen = opaqueToObject<Screen>(self);
        uint32_t color = 0x0;
        if (argc) {
            JSValue const& cVal = argv[0];
            if (!JSValueToScreenColor(ctx, screen, cVal, color)) {
                const char* names[] = {"setColor", "clear"};
                return JS_ThrowTypeError(ctx, "%s: Cannot get color from argument", names[magic]); 
            }
        }
        switch (magic) {
            case 0: screen->setColor(color); break;
            case 1: screen->clear(color); break;
        }
        MAYBE_RETHROW_EXCEPTION_V(ctx, JS_UNDEFINED);
    }

    static JSValue ScreenProto_fillRect(JSContext* ctx, JSValueConst self, int argc, JSValueConst *argv) {
        auto* screen = opaqueToObject<Screen>(self);
        if (argc >= 4) {
            double x; JS_ToFloat64(ctx, &x, argv[1]);
            double y; JS_ToFloat64(ctx, &y, argv[2]);
            double w; JS_ToFloat64(ctx, &w, argv[3]);
            double h; JS_ToFloat64(ctx, &h, argv[4]);
            uint32_t color = screen->getColor();
            if (argc >= 5 && !JSValueToScreenColor(ctx, screen, argv[4], color))
                return JS_ThrowTypeError(ctx, "%s: Cannot parse color.", "fillRect");
            screen->setColor(color);
            screen->fillRect(x, y, w, h);
        }
        MAYBE_RETHROW_EXCEPTION_V(ctx, JS_UNDEFINED);
    }

    static JSValue ScreenProto_drawText(JSContext* ctx, JSValueConst self, int argc, JSValueConst *argv) {
        auto* screen = opaqueToObject<Screen>(self);
        if (argc >= 4) {
            char const* text = JS_ToCString(ctx, argv[0]);
            double x; JS_ToFloat64(ctx, &x, argv[1]);
            double y; JS_ToFloat64(ctx, &y, argv[2]);
            double s; JS_ToFloat64(ctx, &s, argv[3]);
            uint32_t color = screen->getColor();
            if (argc >= 5 && !JSValueToScreenColor(ctx, screen, argv[4], color))
                return JS_ThrowTypeError(ctx, "%s: Cannot parse color.", "drawText");
            screen->setColor(color);
            screen->drawText(text, x, y, s);
            JS_FreeCString(ctx, text);
        }
        MAYBE_RETHROW_EXCEPTION_V(ctx, JS_UNDEFINED);
    }

    static JSValue ScreenProto_drawSprite(JSContext* ctx, JSValueConst self, int argc, JSValueConst *argv) {
        auto* screen = opaqueToObject<Screen>(self);
        if (argc < 5)
            return JS_ThrowTypeError(ctx, "drawSprite expects 5 arguments, %d given.", argc);
        
        char const* sprite = JS_ToCString(ctx, argv[0]);
        double x; JS_ToFloat64(ctx, &x, argv[1]);
        double y; JS_ToFloat64(ctx, &y, argv[2]);
        double w; JS_ToFloat64(ctx, &w, argv[3]);
        double h; JS_ToFloat64(ctx, &h, argv[4]);
        screen->drawSprite(sprite, x, y, w, h);
        JS_FreeCString(ctx, sprite);
        MAYBE_RETHROW_EXCEPTION_V(ctx, JS_UNDEFINED);
    }

    static JSValue ScreenProto_setDrawAnchor(JSContext* ctx, JSValueConst self, int argc, JSValueConst *argv) {
        auto* screen = opaqueToObject<Screen>(self);
        double x; JS_ToFloat64(ctx, &x, argv[0]);
        double y; JS_ToFloat64(ctx, &y, argv[1]);
        screen->setDrawAnchor(x, y);
        MAYBE_RETHROW_EXCEPTION_V(ctx, JS_UNDEFINED);
    }

    static JSCFunctionListEntry defineScreen[] = {
        JS_CFUNC_MAGIC_DEF("startControl", 0, ScreenProto_simple, 0),
        JS_CFUNC_MAGIC_DEF("initContext", 0, ScreenProto_simple, 1),
        JS_CFUNC_MAGIC_DEF("initDraw", 0, ScreenProto_simple, 2),
        JS_CFUNC_MAGIC_DEF("closeDrawOp", 0, ScreenProto_simple, 3),
        JS_CFUNC_MAGIC_DEF("resize", 0, ScreenProto_simple, 4),
        JS_CFUNC_MAGIC_DEF("setColor", 1, ScreenProto_colorArg, 0),
        JS_CFUNC_MAGIC_DEF("clear", 1, ScreenProto_colorArg, 1),
        JS_CFUNC_DEF("fillRect", 5, ScreenProto_fillRect),
        JS_CFUNC_DEF("drawText", 5, ScreenProto_drawText),
        JS_CFUNC_DEF("drawSprite", 5, ScreenProto_drawSprite),
        JS_CFUNC_DEF("setDrawAnchor", 2, ScreenProto_setDrawAnchor),
    };

    static JSValue constructScreen(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
        JSValue val = constructObject<Screen, classId_Screen>(ctx, new_target, argc, argv);
        JSValueConst& rtVal = argv[0];
        if (argc && JS_GetClassID(rtVal) == getRuntimeClassID()) {
            Runtime* rt = opaqueToObject<Runtime>(rtVal);
            if (rt) JS_DupValue(ctx, rtVal);
            opaqueToObject<Screen>(val)->setRuntime(rt);
        } else {
            JS_FreeValue(ctx, val);
            return JS_ThrowInternalError(ctx, "%s", "Trying to construct Screen without Runtime!");
        }
        return val;
    }

    static void gcMarkScreen(JSRuntime* rt, JSValueConst self, JS_MarkFunc markFunc) {
        auto* screen = opaqueToObject<Screen>(self);
        if (screen->getRuntime())
            JS_MarkValue(rt, ScriptProxy<Runtime>::toValue(screen->getRuntime()), markFunc);
    }

    static void destructScreen(JSRuntime* rt, JSValue self) {
        auto* screen = opaqueToObject<Screen>(self);
        if (Runtime* runtime = screen->getRuntime()) {
            runtime->setScreen(nullptr);
            screen->setRuntime(nullptr);
            JS_FreeValueRT(rt, ScriptProxy<Runtime>::toValue(runtime));
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
#pragma region Runtime

    static JSClassID classId_Runtime;

    static JSClassID getRuntimeClassID() { return classId_Runtime; }

    static JSValue constructRuntime(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
        JSValue rtValue = constructObject<Runtime, classId_Runtime>(ctx, new_target, argc, argv);
        Runtime* rt = opaqueToObject<Runtime>(rtValue);
        // fields injected into other JS objects
        JS_SetPropertyStr(ctx, rtValue, "update_memory", JS_NewObject(ctx));
        JS_SetPropertyStr(ctx, rtValue, "touch", JS_NewObject(ctx));
        // script hooks
        rt->startVM.connect([ctx, rtValue]() {
            JSTempVal startFn = JS_GetPropertyStr(ctx, rtValue, "__startReady");
            JSTempVal ret = JS_Call(ctx, startFn, rtValue, 0, nullptr);
        });
        rt->timerStep.connect([ctx, rtValue]() {
            JSTempVal startFn = JS_GetPropertyStr(ctx, rtValue, "__timer");
            JSTempVal ret = JS_Call(ctx, startFn, rtValue, 0, nullptr);
        });
        rt->updatedControls.connect([ctx, rtValue]() {
            Runtime* rt = opaqueToObject<Runtime>(rtValue);
            JSTempVal touch = JS_GetPropertyStr(ctx, rtValue, "touch");
            JS_SetPropertyStr(ctx, touch, "touching", JS_NewBool(ctx, rt->isTouching()));
        });
        JSValue screen = constructScreen(ctx, JS_UNDEFINED, 1, &rtValue);
        rt->setScreen(opaqueToObject<Screen>(screen));
        return rtValue;
    }

    static void destructRuntime(JSRuntime* rt, JSValue self) {
        Runtime* runtime = opaqueToObject<Runtime>(self);
        if (Screen* screen = runtime->getScreen()) {
            screen->setRuntime(nullptr);
            runtime->setScreen(nullptr);
            JS_FreeValueRT(rt, ScriptProxy<Screen>::toValue(screen));
        }
        destructObject<Runtime>(rt, self);
    }

    static JSValue RuntimeProto_screen(JSContext* ctx, JSValueConst self) {
        Runtime* runtime = opaqueToObject<Runtime>(self);
        return JS_DupValue(ctx, ScriptProxy<Screen>::toValue(runtime->getScreen()));
    }

    static JSValue RuntimeProto_simple(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv, int magic) {
        Runtime* runtime = opaqueToObject<Runtime>(self);
        switch (magic) {
            case 1: runtime->exit(); break;
            case 2: runtime->start(); break;
            case 3: runtime->startReady(); break;
            case 4: runtime->checkStartReady(); break;
            case 5: runtime->timer(); break;
            case 6: runtime->updateControls(); break;
        }
        MAYBE_RETHROW_EXCEPTION_V(ctx, JS_UNDEFINED);
    }

    static void gcMarkRuntime(JSRuntime* rt, JSValueConst self, JS_MarkFunc markFunc) {
        Runtime* runtime = opaqueToObject<Runtime>(self);
        if (Screen* screen = runtime->getScreen())
            JS_MarkValue(rt, ScriptProxy<Screen>::toValue(screen), markFunc);
    }

    static JSCFunctionListEntry defineRuntime[] = {
        JS_CGETSET_DEF("screen", RuntimeProto_screen, nullptr),
        JS_CFUNC_MAGIC_DEF("exit", 0, RuntimeProto_simple, 1),
        JS_CFUNC_MAGIC_DEF("start", 0, RuntimeProto_simple, 2),
        JS_CFUNC_MAGIC_DEF("startReady", 0, RuntimeProto_simple,3),
        JS_CFUNC_MAGIC_DEF("checkStartReady", 0, RuntimeProto_simple, 4),
        JS_CFUNC_MAGIC_DEF("timer", 0, RuntimeProto_simple, 5),
        JS_CFUNC_MAGIC_DEF("updateControls", 0, RuntimeProto_simple, 6),
    };

    static void installRuntime(JSContext* ctx) {
        JSClassDef cdef{ .class_name = "Runtime", .finalizer = destructRuntime, .gc_mark = gcMarkRuntime };
        JS_NewClassID(&classId_Runtime);
        JS_NewClass(JS_GetRuntime(ctx), classId_Runtime, &cdef);
        JSValue proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, proto, defineRuntime, countof(defineRuntime));
        // global constructor
        JSValue ctor = JS_NewCFunction2(ctx, constructRuntime,
            cdef.class_name, 1, JS_CFUNC_constructor, 0);
        JS_SetConstructor(ctx, ctor, proto);
        JS_SetClassProto(ctx, classId_Runtime, proto);
        JSTempVal globalThis = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, globalThis, cdef.class_name, ctor);
    }

#pragma endregion
#pragma region Player

    static JSClassID classId_Player;

    static void gcMarkPlayer(JSRuntime* rt, JSValueConst self, JS_MarkFunc markFunc) {
        auto* player = opaqueToObject<Player>(self);
        if (Runtime* runtime = player->getRuntime())
            JS_MarkValue(rt, ScriptProxy<Runtime>::toValue(runtime), markFunc);
    }

    static void destructPlayer(JSRuntime* rt, JSValueConst self) {
        auto* player = opaqueToObject<Player>(self);
        if (Runtime* runtime = player->getRuntime()) {
            JS_FreeValueRT(rt, ScriptProxy<Runtime>::toValue(runtime));
            player->setRuntime(nullptr);
        }
        destructObject<Player>(rt, self);
    }

    static JSValue PlayerProto_start(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) {
        auto* player = opaqueToObject<Player>(self);
        JS_SetPropertyStr(ctx, self, "sources", JS_NewObject(ctx));
        // construct runtime using proxy exposed to JS, so that it sees (and collects) this object as well
        JSValue rtValue = constructRuntime(ctx, JS_UNDEFINED, 0, nullptr);
        player->setRuntime(opaqueToObject<Runtime>(rtValue));
        player->sourceFileAdded.connect([ctx, self](std::string name, std::string text){
            JSTempVal startFn = JS_GetPropertyStr(ctx, self, "__sourceFileAdded");
            JSTempVal values[] = {
                JS_NewStringLen(ctx, name.c_str(), name.length()),
                JS_NewStringLen(ctx, text.c_str(), text.length())};
            JSTempVal ret = JS_Call(ctx, startFn, self, 2, reinterpret_cast<JSValue*>(values));
            MAYBE_RETHROW_EXCEPTION_V(ctx, JS_UNDEFINED);
        });
        player->start();
        MAYBE_RETHROW_EXCEPTION_V(ctx, JS_UNDEFINED);
    }

    static JSValue PlayerProto_runtime(JSContext* ctx, JSValueConst self) {
        Player* player = opaqueToObject<Player>(self);
        return JS_DupValue(ctx, ScriptProxy<Runtime>::toValue(player->getRuntime()));
    }

    static JSCFunctionListEntry definePlayer[] = {
        JS_CFUNC_DEF("start", 0, PlayerProto_start),
        JS_CGETSET_DEF("runtime", PlayerProto_runtime, nullptr)
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
        auto output = [](JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv, int magic) -> JSValue {
            char const* fnName[] = {"INFO", "LOG", "ERROR"};
            LOG_MSGF("[CONSOLE.%s] ", fnName[magic]);
            for (int i = 0; i < argc; ++i) {
                char const* str = JS_ToCString(ctx, argv[i]);
                LOG_MSGF("%s ", str);
                JS_FreeCString(ctx, str);
            }
            LOG_MSG("\n");
            return JS_UNDEFINED;
        };
        JSValue log = JS_NewCFunctionMagic(ctx, output, "log", 1, JSCFunctionEnum::JS_CFUNC_generic_magic, 0);
        JSValue info = JS_NewCFunctionMagic(ctx, output, "info", 1, JSCFunctionEnum::JS_CFUNC_generic_magic, 1);
        JSValue error = JS_NewCFunctionMagic(ctx, output, "error", 1, JSCFunctionEnum::JS_CFUNC_generic_magic, 2);
        JS_SetPropertyStr(ctx, console, "log", log);
        JS_SetPropertyStr(ctx, console, "info", info);
        JS_SetPropertyStr(ctx, console, "error", error);
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
        JSValue browserRef = JS_NewInt64(ctx, reinterpret_cast<int64_t>(m_browser));
        JSValue reqFrame = JS_NewCFunctionData(ctx, 
        [](JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv, int, JSValue* data) -> JSValue {
            if (argc && JS_IsFunction(ctx, argv[0])) {
                JSValue ownFn = JS_DupValue(ctx, argv[0]);
                Context* browser = nullptr; JS_ToInt64(ctx, (int64_t*)&browser, data[0]);
                browser->addRepaintListener([ctx, ownFn](){
                    JSTempVal tstamp = JS_NewObject(ctx); /** TODO: DOMHighResTimeStamp!!! */
                    JSTempVal ret = JS_Call(ctx, ownFn, JS_UNDEFINED, 1, tstamp.ptr());
                }, [ctx, ownFn](){
                    JS_FreeValue(ctx, ownFn);
                });
                return JS_UNDEFINED;
            }
            /** TODO: this should be a counter system? */
            return JS_ThrowTypeError(ctx, "%s arg in requestAnimationFrame", argc? "not a Function" : "expected 1");
        }, 1, 0, 1, &browserRef);
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
