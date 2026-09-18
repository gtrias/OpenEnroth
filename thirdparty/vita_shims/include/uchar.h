/* <uchar.h> shim for PS Vita newlib. Implementations in vita_shims/uchar_shim.c. */
#ifndef _VITA_UCHAR_H_SHIM
#define _VITA_UCHAR_H_SHIM

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
/* char16_t / char32_t are keywords in C++. */
#endif

#ifndef __cplusplus
typedef uint_least16_t char16_t;
typedef uint_least32_t char32_t;
#endif

size_t mbrtoc16(char16_t *restrict, const char *restrict, size_t, mbstate_t *restrict);
size_t c16rtomb(char *restrict, char16_t, mbstate_t *restrict);
size_t mbrtoc32(char32_t *restrict, const char *restrict, size_t, mbstate_t *restrict);
size_t c32rtomb(char *restrict, char32_t, mbstate_t *restrict);

#ifdef __cplusplus
}
namespace std {
using ::mbrtoc16;
using ::c16rtomb;
using ::mbrtoc32;
using ::c32rtomb;
}
#endif

#endif
