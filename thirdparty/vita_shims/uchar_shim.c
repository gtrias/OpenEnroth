/* C11 <uchar.h> conversion shims for PS Vita newlib, which ships none of them.
 *
 * Multibyte encoding is assumed to be UTF-8. These are adequate for OpenEnroth's
 * use (text encoding conversion via ztd.text/ztd.cuneicode), not a full locale
 * implementation. Conversion state for the 16-bit functions packs a pending
 * UTF-16 surrogate into mbstate_t's __count/__c fields.
 */
#include <uchar.h>
#include <wchar.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

size_t mbrtoc16(char16_t *restrict pc16, const char *restrict s, size_t n, mbstate_t *restrict ps) {
	static mbstate_t internal = {0};
	if (ps == NULL) ps = &internal;

	/* Pending low surrogate from a previous call. */
	if ((ps->__count & 0xFFFF0000u) != 0) {
		if (pc16) *pc16 = (char16_t)(ps->__count >> 16);
		ps->__count = 0;
		return (size_t)-3; /* sequence produced */
	}
	if (s == NULL) {
		/* Reset. */
		memset(ps, 0, sizeof(*ps));
		return 0;
	}

	/* Decode one UTF-8 code point. */
	uint32_t cp = 0;
	size_t need = 0;
	unsigned char c0 = (unsigned char)s[0];
	if (c0 < 0x80) { cp = c0; need = 1; }
	else if ((c0 & 0xE0) == 0xC0) { cp = c0 & 0x1F; need = 2; }
	else if ((c0 & 0xF0) == 0xE0) { cp = c0 & 0x0F; need = 3; }
	else if ((c0 & 0xF8) == 0xF0) { cp = c0 & 0x07; need = 4; }
	else return (size_t)-1; /* EILSEQ */

	if (n < need) return (size_t)-2; /* incomplete */

	for (size_t i = 1; i < need; i++) {
		unsigned char ci = (unsigned char)s[i];
		if ((ci & 0xC0) != 0x80) return (size_t)-1;
		cp = (cp << 6) | (ci & 0x3F);
	}
	if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return (size_t)-1;

	if (cp >= 0x10000) {
		uint32_t v = cp - 0x10000;
		char16_t hi = (char16_t)(0xD800 + (v >> 10));
		char16_t lo = (char16_t)(0xDC00 + (v & 0x3FF));
		if (pc16) *pc16 = hi;
		ps->__count = ((uint32_t)lo << 16) | 1;
		return need;
	}
	if (pc16) *pc16 = (char16_t)cp;
	return cp == 0 ? 0 : need;
}

size_t c16rtomb(char *restrict s, char16_t c16, mbstate_t *restrict ps) {
	static mbstate_t internal = {0};
	if (ps == NULL) ps = &internal;
	if (s == NULL) {
		memset(ps, 0, sizeof(*ps));
		return 1;
	}

	uint32_t cp;
	if ((c16 & 0xFC00) == 0xD800) {
		ps->__value.__wch = c16; /* remember high surrogate */
		return 0;
	}
	if ((c16 & 0xFC00) == 0xDC00) {
		uint32_t hi = (uint32_t)(uint16_t)ps->__value.__wch;
		if ((hi & 0xFC00) != 0xD800) return (size_t)-1;
		cp = 0x10000 + ((hi & 0x3FF) << 10) + (c16 & 0x3FF);
	} else {
		cp = c16;
	}
	ps->__value.__wch = 0;

	unsigned char *out = (unsigned char *)s;
	if (cp < 0x80) { out[0] = (unsigned char)cp; return 1; }
	if (cp < 0x800) { out[0] = (unsigned char)(0xC0 | (cp >> 6)); out[1] = (unsigned char)(0x80 | (cp & 0x3F)); return 2; }
	if (cp < 0x10000) { out[0] = (unsigned char)(0xE0 | (cp >> 12)); out[1] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F)); out[2] = (unsigned char)(0x80 | (cp & 0x3F)); return 3; }
	out[0] = (unsigned char)(0xF0 | (cp >> 18)); out[1] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
	out[2] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F)); out[3] = (unsigned char)(0x80 | (cp & 0x3F));
	return 4;
}

size_t mbrtoc32(char32_t *restrict pc32, const char *restrict s, size_t n, mbstate_t *restrict ps) {
	static mbstate_t internal = {0};
	if (ps == NULL) ps = &internal;
	if (s == NULL) {
		memset(ps, 0, sizeof(*ps));
		return 0;
	}

	uint32_t cp = 0;
	size_t need = 0;
	unsigned char c0 = (unsigned char)s[0];
	if (c0 < 0x80) { cp = c0; need = 1; }
	else if ((c0 & 0xE0) == 0xC0) { cp = c0 & 0x1F; need = 2; }
	else if ((c0 & 0xF0) == 0xE0) { cp = c0 & 0x0F; need = 3; }
	else if ((c0 & 0xF8) == 0xF0) { cp = c0 & 0x07; need = 4; }
	else return (size_t)-1;

	if (n < need) return (size_t)-2;
	for (size_t i = 1; i < need; i++) {
		unsigned char ci = (unsigned char)s[i];
		if ((ci & 0xC0) != 0x80) return (size_t)-1;
		cp = (cp << 6) | (ci & 0x3F);
	}
	if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return (size_t)-1;
	if (pc32) *pc32 = cp;
	return cp == 0 ? 0 : need;
}

size_t c32rtomb(char *restrict s, char32_t c32, mbstate_t *restrict ps) {
	static mbstate_t internal = {0};
	if (ps == NULL) ps = &internal;
	if (s == NULL) {
		memset(ps, 0, sizeof(*ps));
		return 1;
	}
	uint32_t cp = (uint32_t)c32;
	if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return (size_t)-1;
	unsigned char *out = (unsigned char *)s;
	if (cp < 0x80) { out[0] = (unsigned char)cp; return 1; }
	if (cp < 0x800) { out[0] = (unsigned char)(0xC0 | (cp >> 6)); out[1] = (unsigned char)(0x80 | (cp & 0x3F)); return 2; }
	if (cp < 0x10000) { out[0] = (unsigned char)(0xE0 | (cp >> 12)); out[1] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F)); out[2] = (unsigned char)(0x80 | (cp & 0x3F)); return 3; }
	out[0] = (unsigned char)(0xF0 | (cp >> 18)); out[1] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
	out[2] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F)); out[3] = (unsigned char)(0x80 | (cp & 0x3F));
	return 4;
}
