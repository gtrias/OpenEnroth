/* sys/mman.h shim for PS Vita newlib: implements anonymous & file-backed
 * private mappings over malloc/pread. Sufficient for mio (OpenEnroth asset IO)
 * and LuaJIT's lj_alloc. */
#ifndef _VITA_SYS_MMAN_SHIM
#define _VITA_SYS_MMAN_SHIM

#include <sys/types.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 0x20
#define MAP_ANON MAP_ANONYMOUS
#define MAP_FAILED ((void *)-1)

#define MS_ASYNC 1
#define MS_SYNC 2
#define MS_INVALIDATE 4

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
int mprotect(void *addr, size_t len, int prot);
int msync(void *addr, size_t length, int flags);
int mlock(const void *addr, size_t len);
int munlock(const void *addr, size_t len);

#ifdef __cplusplus
}
#endif

#endif
