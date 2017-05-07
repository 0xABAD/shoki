#ifndef GUARD__COMMON_H__
#define GUARD__COMMON_H__

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  f32;
typedef double f64;

#define COUNT_OF(arr) ((sizeof(arr) / sizeof(0[arr])) / ((size_t)(!(sizeof(arr) % sizeof(0[arr])))))

/*
 * Go's defer statement in C++ with a slight caveat in semantics (see credits).
 * Defer has an advantage of where you don't need to wrap initialization/cleanup 
 * code in a class, you clearly see the initialization/cleanup pair, and works
 * better with C code bases.
 *
 * Note this requires C++11 or greater.
 *
 * Credits for implementation go to Ginger Bill:
 *
 * http://www.gingerbill.org/article/defer-in-cpp.html
 */
template <typename Fn>
struct PrivateDeferStruct {
    Fn deferCallback;

    PrivateDeferStruct(Fn fn) : deferCallback(fn) {}
    ~PrivateDeferStruct() { deferCallback(); }

    PrivateDeferStruct(const PrivateDeferStruct &)             = delete;
    PrivateDeferStruct& operator=(const PrivateDeferStruct &)  = delete;

    PrivateDeferStruct(PrivateDeferStruct &&)            = default;
    PrivateDeferStruct& operator=(PrivateDeferStruct &&) = default;
};

template <typename Fn>
PrivateDeferStruct<Fn>
privateDeferFunction(Fn fn)
{
    return PrivateDeferStruct<Fn>(fn);
}

#define DEFER_0(x, y) x##y
#define DEFER_1(x, y) DEFER_0(x, y)
#define DEFER_2(x)    DEFER_1(x, __COUNTER__)

#define defer(code) auto DEFER_2(_local_defer_) = privateDeferFunction([&](){code;})

#endif // GUARD__COMMON_H__

