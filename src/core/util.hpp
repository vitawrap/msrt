#pragma once

#include <assert.h>

#define SAFE_DELETE(f) if (f) { delete(f); f = nullptr; }

#ifndef NDEBUG
#define DEBUG_ASSERT(x) assert(x)
#else
#define DEBUG_ASSERT(x)
#endif //NDEBUG


// Platform stuff

// Thanks, microsoft
#ifdef VSFIX
#ifndef __attribute__
enum class __attr_e
{
    always_inline,
    __always_inline__,
    packed,
    __packed__,
    deprecated,
    __deprecated__
};
#define __attr__arg__(x)
#define __attribute__(x) __attr__arg__(__attr_e:: x)
#endif //__attribute__
#endif //VSFIX

#if defined(__GNUC__) || defined(__clang__)
#define PACKED __attribute__((packed))
#define FORCEINLINE       inline __attribute__((always_inline))
#define FUNC_FA_PRINTFLIKE(f, a)   __attribute__((format(printf, f, a)))
#else
#define FORCEINLINE  inline
#define PACKED /* TODO */
#define FUNC_FA_PRINTFLIKE(f, a)
#endif

#ifndef _WIN32
#define _stricmp strcasecmp
#define _strdup strdup
#endif

// Math stuff

constexpr unsigned blog2_itr(unsigned n)
{
    unsigned log = 0;
    while (n >>= 1)
        ++log;
    return log;
}

constexpr unsigned blog2(unsigned v) // Preferred (faster, 32-bit)
{
	// find the log base 2 of 32-bit v
	const int MultiplyDeBruijnBitPosition[32] =
	{
	  0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
	  8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
	};

	v |= v >> 1; // first round down to one less than a power of 2 
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;

	return MultiplyDeBruijnBitPosition[(unsigned)(v * 0x07C4ACDDU) >> 27];
}

// Works for powers of 2
constexpr unsigned bsqrt(unsigned n)
{
    return n >> (blog2(n) >> 1);
}

// Rotate left 32bit unsigned integer
#define ROTL(x, b) (uint32_t)(((x) << (b)) | ((x) >> (32 - (b))))

// RAD TO DEG float
#define RAD_TO_DEG_F 57.29578f

namespace ms {
    namespace Time
    {
        // Get current time in seconds
        extern double now();
        // Get current time in seconds as of this frame (faster)
        extern double frameNow();
    
        // Must only be called once per frame!
        extern void updateFrameNow();
    }
    
    // Threading extras
    namespace Thread
    {
        extern bool isMainThread();
    
        extern void setMainThread();
    }
}


#define SCOPE_LOCK(mtx) std::scoped_lock scopedlock(mtx)
