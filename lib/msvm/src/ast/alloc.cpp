#include "alloc.hpp"

#include <malloc.h>
#include <memory.h>
#include <ms/core/util.hpp>

namespace vm {
namespace ast {
    thread_local Alloc::MemoryBlock* Alloc::m_head = nullptr;
    thread_local Alloc::MemoryBlock* Alloc::m_tail = nullptr;

    void* Alloc::allocBlock(size_t sz, const void* dtor) {
        size_t realSize = sz + sizeof(MemoryBlock);
        MemoryBlock* mem = (MemoryBlock*) malloc(realSize);
        mem->next = nullptr;
        mem->dtor = dtor;

        if (m_tail)
            m_tail->next = mem;
        else
            m_head = mem;
        m_tail = mem;

        return (void*) mem->data;
    }

    void Alloc::reset() {
        free();
    }

    void Alloc::free() {
        typedef void (*TGenericDeleter) (void* thisptr);

        MemoryBlock* block = m_head;
        while (block)
        {
            MemoryBlock* temp = block->next;
            if (block->dtor)
                ((TGenericDeleter)(block->dtor))(block->data);
            ::free((void*) block);
            block = temp;
        }
        m_head = nullptr;
        m_tail = nullptr;
    }

    size_t Alloc::inplaceEscape(char* str) {
        size_t ofs = 0;
        while (str[ofs]) {
            if (str[ofs] == '\\') {
                // Escaped single "\".
                if (str[ofs+1] == '\\')
                    ++ofs;
                // Escaped newline.
                else if (str[ofs+1] == 'n')
                    str[++ofs] = '\n';
                // Escaped carriage.
                else if (str[ofs+1] == 'r')
                    str[++ofs] = '\r';
                // Escaped tab.
                else if (str[ofs+1] == 't')
                    str[++ofs] = '\t';
            }
            str[0] = str[ofs];
            ++str;
        }
        str[0] = str[ofs];  // also copy NT
        return ofs;
    }

    char* Alloc::strdup(const char *str, bool escape) {
        size_t sz = 1;
        const char* ptr = str;
        while (*ptr++) ++sz;
        if (escape)
            sz -= inplaceEscape(const_cast<char*>(str));

        void* block = allocBlock(sz);
        memcpy(block, str, sz);
        return (char*) block;
    }

    void* COWPtr::allocBlock(size_t sz, const void* dtor) {
        size_t realSize = sz + sizeof(COWHeader);
        COWHeader* mem = (COWHeader*) malloc(realSize);
        mem->refc = 1;
        mem->size = sz;
        mem->dtor = dtor;

        return (void*) mem->data;
    }

    void const* COWPtr::ref(void const* cowptr) {
        DEBUG_ASSERT(cowptr);
        COWHeader* header = reinterpret_cast<COWHeader*>(const_cast<void*>(cowptr)) - 1;
        ++(header->refc);
        return cowptr;
    }

    std::nullptr_t COWPtr::unref(void *cowptr) {
        DEBUG_ASSERT(cowptr);
        typedef void (*TGenericDeleter) (void* thisptr);

        COWHeader* header = reinterpret_cast<COWHeader*>(cowptr) - 1;
        if ((header->refc--) == 1) { // should be reentrant
            if (header->dtor)
                ((TGenericDeleter)(header->dtor))(header->data);
            ::free((void*) header);
        }
        return nullptr;
    }

    void* COWPtr::reallocBlock(void *cowptr, size_t newsz, bool no_src_dtor) {
        if (cowptr) {
            COWHeader* source = reinterpret_cast<COWHeader*>(cowptr) - 1;
            if ((newsz <= source->size) && (newsz > (source->size >> 1)) && (source->refc == 1))
                return cowptr; // no need to reallocate if size is same (or ^half) when refc=1
            void* mem = allocBlock(newsz, source->dtor);
            ::memcpy(mem, source->data, source->size); // realloc behavior
            if (no_src_dtor) source->dtor = nullptr;
            COWPtr::unref(cowptr); // unref source block
            return mem;
        }
        return allocBlock(newsz);
    }
}
}