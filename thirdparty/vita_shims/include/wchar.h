/* <wchar.h> wrapper for Vita newlib: adds the C11 <uchar.h> declarations that
 * glibc also exposes here (ztd.cuneicode relies on this). */
#ifndef _VITA_WCHAR_SHIM
#define _VITA_WCHAR_SHIM
#include_next <wchar.h>

#ifndef _VITA_UCHAR_H_SHIM
#ifdef __cplusplus
extern "C" {
#endif
size_t mbrtoc16(char16_t *__restrict pc16, const char *__restrict s, size_t n, mbstate_t *__restrict ps);
size_t c16rtomb(char *__restrict s, char16_t c16, mbstate_t *__restrict ps);
size_t mbrtoc32(char32_t *__restrict pc32, const char *__restrict s, size_t n, mbstate_t *__restrict ps);
size_t c32rtomb(char *__restrict s, char32_t c32, mbstate_t *__restrict ps);
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

#endif
