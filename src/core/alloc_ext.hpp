#pragma once

#include <stddef.h>

// this makes it easy to rename externs if snmalloc is no longer desired.
#define MS_ALLOC_EXT_FUNC(name) sn_ ## name

extern "C" void* MS_ALLOC_EXT_FUNC(malloc) (size_t size);
extern "C" void* MS_ALLOC_EXT_FUNC(realloc) (void* ptr, size_t size);
extern "C" void* MS_ALLOC_EXT_FUNC(calloc) (size_t nmemb, size_t size);
extern "C" size_t MS_ALLOC_EXT_FUNC(malloc_usable_size) (const void* ptr);
extern "C" void MS_ALLOC_EXT_FUNC(free) (void* ptr);

