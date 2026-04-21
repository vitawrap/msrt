#include "core/app.hpp"
#include "quickjs.h"
#include "script_host.hpp"
#include "script_qjs.hpp"
#include "io/log.hpp"

#include "node.hpp"
#include "nodes/canvas.hpp"
#include "core/events.hpp"

/**
 * All of those methods assume JSTempVal scoping was set up.
 */
namespace ms {
namespace browser {

    /* Classes */

    static JSClassID classId_Node;
    static JSClassID classId_HTMLElement;
    static JSClassID classId_HTMLCanvas;

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

    /* Libraries */

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
        proxy->setValue(obj);
        return obj;
    }

    template <typename T>
    static void destructObject(JSRuntime* rt, JSValue self) {
        auto* proxy = reinterpret_cast<ScriptProxy<Node>*>(JS_GetOpaque(self, JS_GetClassID(self)));
        DEBUG_ASSERT(proxy && "wrong classID in destructObject!");
        delete proxy; // also destroys encapsulated T. Every qjs subclass MUST derive this destructor, since no dynamic binding test will be made.
    }

    template <typename T, JSClassID const& classID>
    static const auto constructNode = constructObject<T, classID>; // node has no specific setup in constructor

    // node (and derived) gc tagging of children
    static void gcMarkNode(JSRuntime* rt, JSValueConst v, JS_MarkFunc mfn) {
        void* opaque = JS_GetOpaque(v, JS_GetClassID(v));
        DEBUG_ASSERT(opaque && "wrong classID in gcMarkNode!");
        auto const* ptr = ScriptProxy<Node>::cast(opaque);
        for (const auto* child : *ptr) // (recursively) assumes ALL children are allocated by JS!!
            JS_MarkValue(rt, ScriptProxy<Node>::toValue(child), mfn);
    }

    template <typename T, JSClassID const& classID>
    static const auto constructElement = constructNode<T, classID>; /** TODO: element-specific construction */

    static void installNodes(JSContext* ctx) {
        JSClassDef cdef;
        cdef.class_name = "Node";
        cdef.finalizer = &destructObject<Node>;
        cdef.gc_mark = &gcMarkNode;
        JS_NewClassID(&classId_HTMLCanvas);
        JS_NewClass(JS_GetRuntime(ctx), classId_HTMLCanvas, &cdef);
        JSValue proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, proto, nullptr, 0);
        // global constructor
        JSValue ctor = JS_NewCFunction2(ctx, constructNode<Node, classId_Node>, 
            cdef.class_name, 0, JS_CFUNC_constructor, 0);
        JS_SetConstructor(ctx, ctor, proto);
        JS_SetClassProto(ctx, classId_HTMLCanvas, proto);
        JSTempVal globalThis = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, globalThis, cdef.class_name, ctor);
    }

    void ScriptHost::installLibCanvas() {
        auto* ctx = m_engine->context;
        JSClassDef cdef;
        cdef.class_name = "HTMLCanvasElement";
        cdef.finalizer = &destructObject<HTMLCanvasElement>;
        cdef.gc_mark = &gcMarkNode;
        JS_NewClassID(&classId_HTMLCanvas);
        JS_NewClass(JS_GetRuntime(ctx), classId_HTMLCanvas, &cdef);
        JSValue proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, proto, nullptr, 0);
        // global constructor
        JSValue ctor = JS_NewCFunction2(ctx, [](JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) -> JSValue {
            JSValue base = constructElement<HTMLCanvasElement, classId_HTMLCanvas>(ctx, new_target, argc, argv);
            return base;
        }, cdef.class_name, 0, JS_CFUNC_constructor, 0);

        JS_SetConstructor(ctx, ctor, proto);
        JS_SetClassProto(ctx, classId_HTMLCanvas, proto);
        JSTempVal globalThis = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, globalThis, cdef.class_name, ctor);
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
            return JS_UNDEFINED; /** TODO: this should be a counter system? */
        }, "requestAnimationFrame", 1);
        JS_SetPropertyStr(ctx, globalThis, "requestAnimationFrame", reqFrame);

        // re-add globalThis as window
        JS_SetPropertyStr(ctx, globalThis, "window", globalThis);



        // install conventional objects
        installConsole(ctx);

        // install basic nodes
        installNodes(ctx);
    }

}
}