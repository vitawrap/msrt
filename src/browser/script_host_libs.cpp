#include "core/app.hpp"
#include "script_host.hpp"
#include "script_qjs.hpp"
#include "io/log.hpp"

#include "node.hpp"
#include "nodes/canvas.hpp"
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
#pragma region Event

    static JSClassID classId_Event;

    static void installEvents(JSContext* ctx) {
        JSClassDef cdef{ .class_name = "HTMLEvent", .finalizer = &destructObject<HTMLEvent>, .gc_mark = nullptr };
        JS_NewClassID(&classId_Event);
        JS_NewClass(JS_GetRuntime(ctx), classId_Event, &cdef);
        JSValue proto = JS_NewObject(ctx);
        // global constructor
        JSValue ctor = JS_NewCFunction2(ctx, constructObject<HTMLEvent, classId_Event>, 
            cdef.class_name, 0, JS_CFUNC_constructor, 0);
        JS_SetConstructor(ctx, ctor, proto);
        JS_SetClassProto(ctx, classId_Event, proto);
        JSTempVal globalThis = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, globalThis, cdef.class_name, ctor);
    }

#pragma endregion
#pragma region Nodes

    static JSClassID classId_Node;

    template <typename T, JSClassID const& classID>
    static const auto constructNode = constructObject<T, classID>; // node has no specific setup in constructor

    template <typename T>
    static void destructNode(JSRuntime* rt, JSValue self) {
        auto* parent = opaqueToObject<T>(self);
        for (auto* child : *parent) {
            parent->removeChild(child); // remove children and decrement ownership ref
            JS_FreeValueRT(rt, ScriptProxy<Node>::toValue(child));
        }
        destructObject<T>(rt, self);
    }

    // node (and derived) gc tagging of children
    static void gcMarkNode(JSRuntime* rt, JSValueConst v, JS_MarkFunc mfn) {
        auto const* ptr = opaqueToObject<Node>(v);
        for (const auto* child : *ptr) // (recursively) assumes ALL children are allocated by JS!!
            JS_MarkValue(rt, ScriptProxy<Node>::toValue(child), mfn);
    }

    static JSValue NodeProto_treeChild(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv, int magic) {
        auto* parent = opaqueToObject<Node>(self);
        JSValueConst& childVal = argv[0];
        auto* child = opaqueToObject<Node>(childVal);
        switch (magic) {
            case 0: // appendChild
            if (parent->appendChild(child))
                JS_DupValue(ctx, childVal); // increment ref
            else {
                JS_ThrowInternalError(ctx, "%s", "Failed to add child node to parent");
                return JS_EXCEPTION;
            }
            break;
            case 1: // removeChild
            if (parent->removeChild(child))
                JS_FreeValue(ctx, childVal); // decrement ref
            else {
                JS_ThrowInternalError(ctx, "%s", "Failed to remove child from parent!");
                return JS_EXCEPTION;
            }
            break;
        }
        return JS_UNDEFINED;
    }

    static JSValue NodeProto_getters(JSContext *ctx, JSValueConst self, int magic) {
        auto* node = opaqueToObject<Node>(self);
        switch (magic) {
            /* parentNode */ case 0: return JS_DupValue(ctx, ScriptProxy<Node>::toValue(node->getParent()));
            /* parentElement */ case 1:
                return dynamic_cast<HTMLElement*>(node->getParent())? JS_DupValue(ctx, ScriptProxy<Node>::toValue(node->getParent())) : JS_NULL;
        }
        return JS_UNDEFINED;
    }

    static JSCFunctionListEntry defineNode[] = {
        JS_CFUNC_MAGIC_DEF("appendChild", 1, NodeProto_treeChild, 0),
        JS_CFUNC_MAGIC_DEF("removeChild", 1, NodeProto_treeChild, 1),
        JS_CGETSET_MAGIC_DEF("parentNode", NodeProto_getters, nullptr, 0),
        JS_CGETSET_MAGIC_DEF("parentElement", NodeProto_getters, nullptr, 1),
    };

    static void installNodes(JSContext* ctx) {
        JSClassDef cdef{ .class_name = "Node", .finalizer = &destructNode<Node>, .gc_mark = &gcMarkNode };
        JS_NewClassID(&classId_Node);
        JS_NewClass(JS_GetRuntime(ctx), classId_Node, &cdef);
        JSValue proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, proto, defineNode, countof(defineNode));
        // global constructor
        JSValue ctor = JS_NewCFunction2(ctx, constructNode<Node, classId_Node>, 
            cdef.class_name, 0, JS_CFUNC_constructor, 0);
        JS_SetConstructor(ctx, ctor, proto);
        JS_SetClassProto(ctx, classId_Node, proto);
        JSTempVal globalThis = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, globalThis, cdef.class_name, ctor);
    }

#pragma endregion
#pragma region Element

    static JSClassID classId_HTMLElement;

    template <typename T, JSClassID const& classID>
    static const auto constructElement = constructNode<T, classID>; /** TODO: element-specific construction */

    static void installElements(JSContext* ctx) {
        JSClassDef cdef{ .class_name = "HTMLElement", .finalizer = &destructNode<HTMLElement>, .gc_mark = &gcMarkNode };
        JS_NewClassID(&classId_HTMLElement);
        JS_NewClass(JS_GetRuntime(ctx), classId_HTMLElement, &cdef);
        JSValue proto = JS_NewObject(ctx);
        if (inheritPrototype(ctx, proto, "Node")) {
            JS_SetPropertyFunctionList(ctx, proto, nullptr, 0);
            JSValue ctor = JS_NewCFunction2(ctx, constructElement<Node, classId_HTMLElement>, 
                cdef.class_name, 0, JS_CFUNC_constructor, 0);
            JS_SetConstructor(ctx, ctor, proto);
            JS_SetClassProto(ctx, classId_HTMLElement, proto);
            JSTempVal globalThis = JS_GetGlobalObject(ctx);
            JS_SetPropertyStr(ctx, globalThis, cdef.class_name, ctor);
            return;
        }        
        JS_FreeValue(ctx, proto);
    }

#pragma endregion
#pragma region Canvas

    static JSClassID classId_HTMLCanvas;

    void ScriptHost::installLibCanvas() {
        auto* ctx = m_engine->context;
        JSClassDef cdef{ .class_name = "HTMLCanvasElement", .finalizer = &destructNode<HTMLCanvasElement>, .gc_mark = &gcMarkNode };
        JS_NewClassID(&classId_HTMLCanvas);
        JS_NewClass(JS_GetRuntime(ctx), classId_HTMLCanvas, &cdef);
        JSValue proto = JS_NewObject(ctx);
        if (inheritPrototype(ctx, proto, "HTMLElement")) {
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
            return;
        }
        JS_FreeValue(ctx, proto);
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
        installEvents(ctx);

        // install basic nodes
        installNodes(ctx);
        installElements(ctx);

        // re-add globalThis as window
        JS_SetPropertyStr(ctx, globalThis, "window", globalThis);
    }

}
}

#pragma clang diagnostic pop
