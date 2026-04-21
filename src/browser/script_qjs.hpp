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
#include <string>

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

        void setValue(JSValue value) { value = m_ownValue; }
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
         * Cast an owned pointer obtained from a foreing env (who shall be from a ScriptProxy<any>*) to its associated value.
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

    struct ScriptCast {        
        template <typename V>
        static V arg(JSValueConst& jsv) {
            return V();
        }

        template <typename V>
        static JSValue ret(JSContext* ctx, V const& ret) {
            if constexpr (std::is_base_of_v<std::string, V>) { // c++ string
                return JS_NewStringLen(ctx, ret.c_str(), ret.length());
            } else if constexpr (std::is_same_v<std::decay_t<V>, const char*>) { // static c string
                return JS_NewString(ctx, ret);
            } else if constexpr (std::is_same_v<bool, V>) { // boolean
                return JS_NewBool(ctx, ret);
            } else if constexpr (std::is_pointer_v<V>) { // pointer to unmanaged data
                static_assert(false, "Casting arbitrary memory to JSValue is not yet possible...");
            } else if constexpr (std::is_integral_v<V>) {
                return JS_NewInt64(ctx, static_cast<int64_t>(ret));
            } else if constexpr (std::is_floating_point_v<V>) {
                return JS_NewFloat64(ctx, static_cast<double>(ret));
            }
            return JS_UNDEFINED;
        }
    };

    /**
     * @brief wildcard internal template for script call into native call
     */
    template <typename R, typename T, typename... Args>
    struct ScriptMethodInternal {
    public:
        template <size_t... IS>
        static JSValue call(R (T::*fptr) (Args...), T* object, JSValueConst* argv, std::index_sequence<IS...> const&) {
            return ScriptCast::ret((object->*fptr)(ScriptCast::arg<Args>(argv[IS])...));
        }
    };

    /**
     * @brief void return type template for script call into native call
     */
    template <typename T, typename... Args>
    struct ScriptMethodInternal<void, T, Args...> {
        template <size_t... IS>
        static JSValue call(void (T::*fptr) (Args...), T* object, JSValueConst* argv, std::index_sequence<IS...> const&) {
            (object->*fptr)(ScriptCast::arg<Args>(argv[IS])...);
            return JS_UNDEFINED;
        }
    };

    /**
     * @brief public part of script method registration
     * this does all the necessary template work to accept bare C++ functions
     */
    struct ScriptMethod {
    public:
        template <JSClassID const& ClassID, typename T, typename R, typename... Args>
        static JSCFunction* from(R (T::*fptr) (Args...)) {
            static auto methodPtr = fptr; // doesn't need thread-safety, should be template-unique
            return [](JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) -> JSValue {
                void* opaque = JS_GetOpaque(self, JS_GetClassID(self));
                DEBUG_ASSERT(opaque && "wrong classID in ScriptMethod::from!");
                T* object = ScriptProxy<T>::cast(opaque);
                constexpr auto is = std::index_sequence_for<Args...>{};
                return ScriptMethodInternal<R, T, Args...>::call(methodPtr, object, argv, is);
            };
        }
    };

}
}

#endif
