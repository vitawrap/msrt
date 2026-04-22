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
#include <quickjs.h>
#include <utility>
#include <cstddef>

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
    protected:
        JSValue m_ownValue;
        JSContext* m_context;
        T m_object;
    public:
        typedef ScriptProxy Self;

        template <typename... Args>
        ScriptProxy(JSContext* ctx, Args&& ...args) :
            m_context(ctx),
            m_object(std::forward<Args>(args)...) {}
        ~ScriptProxy() { JS_FreeValue(m_context, m_ownValue); }

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
            return reinterpret_cast<T*>((char*)ptr + offsetof(ScriptProxy, m_object));
        }

        /**
         * Cast an owned pointer obtained from a foreign env (who shall be from a ScriptProxy<any>*) to its associated value.
         * template typename T can be a base class here.
         */
        static JSValueConst toValue(T const* proxyOwnedObject) {
            if (proxyOwnedObject) {
                char* proxyBase = ((char*)proxyOwnedObject - offsetof(ScriptProxy, m_object));
                return *reinterpret_cast<JSValueConst*>(proxyBase + offsetof(ScriptProxy, m_ownValue));
            }
            return JS_NULL;
        }
    };

}
}

#endif
