/* mmap() implementation over malloc/pread for PS Vita (no MMU syscalls exposed
 * to homebrew). See vita_shims/include/sys/mman.h. */
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
	(void)addr;
	(void)prot;
	if (length == 0) {
		errno = EINVAL;
		return MAP_FAILED;
	}
	void *mem = malloc(length);
	if (mem == NULL) {
		errno = ENOMEM;
		return MAP_FAILED;
	}
	if (fd >= 0 && !(flags & MAP_ANONYMOUS)) {
		char *p = (char *)mem;
		size_t remaining = length;
		while (remaining > 0) {
			ssize_t got = pread(fd, p, remaining, offset);
			if (got < 0) {
				free(mem);
				errno = EIO;
				return MAP_FAILED;
			}
			if (got == 0) break; /* mapping past EOF reads as zeroes */
			p += got;
			offset += got;
			remaining -= (size_t)got;
		}
		if (remaining > 0) memset(p, 0, remaining);
	} else {
		memset(mem, 0, length);
	}
	return mem;
}

int munmap(void *addr, size_t length) {
	(void)length;
	free(addr);
	return 0;
}

int mprotect(void *addr, size_t len, int prot) {
	(void)addr; (void)len; (void)prot;
	return 0; /* no-op: homebrew memory is RWX anyway */
}

int msync(void *addr, size_t length, int flags) {
	(void)addr; (void)length; (void)flags;
	return 0;
}

int mlock(const void *addr, size_t len) { (void)addr; (void)len; return 0; }
int munlock(const void *addr, size_t len) { (void)addr; (void)len; return 0; }
