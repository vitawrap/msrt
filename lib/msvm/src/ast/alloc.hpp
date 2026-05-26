#pragma once

#include <stddef.h>
#include <atomic>

namespace vm {
namespace ast {

    /**
     * @brief Allocator for parser/AST resources
     * Allocate strings and tree nodes via this class
     */
    class Alloc {
        struct MemoryBlock {
            MemoryBlock* next;
            void const* dtor;
            unsigned char data[0];
        };
        static thread_local MemoryBlock *m_head, *m_tail;

        /**
         * Escape a string in-place.
         */
        static size_t inplaceEscape(char* string);

    public:
        /**
         * (Re-)initialize block list.
         */
        static void reset();

        /**
         * Generic block allocation
         */
        static void* allocBlock(size_t sz, void const* dtor = nullptr);

        /**
         * Re-allocate a string and append to list.
         * @param escape if true, escape sequences will be effective.
         */
        static char* strdup(char const* str, bool escape = false);

        /**
         * Free all allocated blocks.
         * (all pointers from this allocator become invalid.)
         */
        static void free();
    };

    /**
     * Allocator for ref-counted COW blocks
     */
    class COWPtr {
        struct COWHeader {
            std::atomic_int32_t refc;
            uint32_t size;
            void const* dtor;
            char data[0];
        };

    public:
        /**
         * Generic block allocation (refcount is initialized at 1)
         */
        static void* allocBlock(size_t sz, void const* dtor = nullptr);

        /**
         * Increment refcount for a COW block
         */
        static void const* ref(void const* cowptr);

        /**
         * Decrement refcount for a COW block
         */
        static std::nullptr_t unref(void* cowptr);

        /**
         * Realloc a COW memory block (write operation)
         * This does not behave like C realloc, in that the source ptr
         * is ONLY freed if realloc causes its refcount to trop to zero.
         */
        static void* reallocBlock(void* cowptr, size_t newsz, bool no_src_dtor = false);
    };

}
}