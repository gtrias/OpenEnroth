#include "VitaMemory.h"

#include <psp2/kernel/sysmem.h>

#include <vitaGL.h>

#include "Library/Logger/Logger.h"

void logVitaMemoryUsage(std::string_view stage) {
    SceKernelFreeMemorySizeInfo info = {};
    info.size = sizeof(SceKernelFreeMemorySizeInfo); // The kernel ignores the call if this isn't set.
    sceKernelGetFreeMemorySize(&info);

    MM_INFO("Memory at {}: vitaGL VRAM (CDRAM) free {} KiB, RAM free {} KiB, phycont free {} KiB; system user free {} KiB.",
            stage,
            vglMemFree(VGL_MEM_VRAM) / 1024,
            vglMemFree(VGL_MEM_RAM) / 1024,
            vglMemFree(VGL_MEM_PHYCONT) / 1024,
            info.size_user / 1024);
}
