/* flockfile/funlockfile for Vita newlib: libc declares them but pthread-embedded
 * doesn't provide the implementations. Stdio locking on vita is a no-op. */
#include <stdio.h>

void flockfile(FILE *fp) { (void)fp; }
void funlockfile(FILE *fp) { (void)fp; }
