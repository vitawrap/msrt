#ifndef _SCRIPT_QJS_HPP_
#define _SCRIPT_QJS_HPP_
/**
 * This header file only exists to provide utility
 * classes to translation units managing the JS state.
 * It also allows script_host.hpp to live without
 * including QuickJS in the main prgoram tree.
 * (Hence the include guard)
 */

#include "core/util.hpp"
#include <utility>
#include <cstddef>
#include <typeinfo>
#include <stdexcept>
#include <string>

// QuickJS includes
#include <cutils.h>
#include <quickjs.h>

namespace ms {
namespace browser {

    /**
     * Box around single JS value to automate free-ing
     */
    struct JSTempVal {
    private:
        static thread_local JSContext* s_context;
        ::JSValue m_value;
    public:
        static void scopeContext(JSContext* ctx) { s_context = ctx; }
        FORCEINLINE JSTempVal(::JSValue&& v) : m_value(std::move(v)) {}
        FORCEINLINE JSTempVal() : m_value(JS_UNDEFINED) {}
        ~JSTempVal() {
            DEBUG_ASSERT(s_context);
            JS_FreeValue(s_context, m_value);
        }
        operator ::JSValue () const { return m_value; }
        JSValue* ptr() { return &m_value; }
    };

    /**
     * Script engine-specific internal structure
     */
    struct ScriptEngine {
        JSRuntime* runtime;
        JSContext* context;
    };

    /**
     * @brief Script interface for an object of arbitrary type.
     * This structure is allocated instead of the raw T object.
     */
    template <typename T>
    class ScriptProxy {
        template <typename FT> friend class ScriptProxy;
    protected:
        JSValue m_ownValue;
        JSRuntime* m_rt;
        T m_object;
    public:
        typedef ScriptProxy Self;

        template <typename... Args>
        ScriptProxy(JSContext* ctx, Args&& ...args) :
            m_rt(JS_GetRuntime(ctx)),
            m_object(std::forward<Args>(args)...) {}
        ~ScriptProxy() { JS_FreeValueRT(m_rt, m_ownValue); }

        void setValue(JSValue value) { m_ownValue = value; }
        JSValueConst getValue() const { return m_ownValue; }
        operator JSValue() { return m_ownValue; }

        T const* getObject() const { return m_object; }
        T* getObject() { return &m_object; }

        T const* operator ->() const { return m_object; }
        T* operator ->() { return &m_object; }

        /**
         * Cast a raw pointer obtained from foreign env (who shall be a ScriptProxy<any>*) to its owned object.
         * template typename T can be a base class here.
         */
        static T* cast(void* ptr) {
            // use int specialization of ScriptProxy otherwise the compiler whines about non-standard layout despite object T being at the end
            return reinterpret_cast<T*>((char*)ptr + offsetof(ScriptProxy<int>, m_object));
        }

        /**
         * Cast an owned pointer obtained from a foreign env (who shall be from a ScriptProxy<any>*) to its associated value.
         * template typename T can be a base class here.
         */
        static JSValueConst toValue(T const* proxyOwnedObject) {
            if (proxyOwnedObject) {
                char* proxyBase = ((char*)proxyOwnedObject - offsetof(ScriptProxy<int>, m_object));
                return *reinterpret_cast<JSValueConst*>(proxyBase + offsetof(ScriptProxy<int>, m_ownValue));
            }
            return JS_NULL;
        }
    };

    /* Util */

    /**
     * Script error on the engine side (QuickJS, ...)
     */
    class ScriptEngineException : public std::runtime_error {
    public:
        ScriptEngineException(char const* err)
            : std::runtime_error(err) {}
    };

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

    // hand exception over to evalScript when possible
    #define MAYBE_RETHROW_EXCEPTION(ctx) if (JS_HasException(ctx)) return JS_EXCEPTION;
    #define MAYBE_RETHROW_EXCEPTION_V(ctx, retv) if (JS_HasException(ctx)) return JS_EXCEPTION; else return retv;

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

}
}

#endif
