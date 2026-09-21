#include "VitaOpenGLContext.h"

#include <cstring>

#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>

#include <vitaGL.h>

#include "Library/Logger/Logger.h"

namespace {
// The Vita's SDL backend doesn't surface system suspend/resume events, and neither does vitaGL - an app that keeps
// rendering into the display queue while the system is going to sleep crashes on wake (that's the crash this port
// showed after suspending the console). Every frame passes through swapBuffers, so this is the natural place to
// quiesce: when the power manager asks for a suspension, park the main thread until the system is back.
void waitForResumeIfSuspended() {
    if (!scePowerIsSuspendRequired())
        return;

    MM_INFO("System suspension requested, halting rendering until resume.");
    do {
        sceKernelDelayThread(100 * 1000); // Don't busy-spin against the power manager.
    } while (scePowerIsSuspendRequired());
    MM_INFO("System resumed, continuing rendering.");
}


// vitaGL doesn't export every entry point that glad-based code calls, and a call through the resulting null pointer
// takes the whole process down. ImGui calls glDetachShader when it tears its device objects down; detaching a shader
// that's about to be deleted anyway is optional, so a no-op is the right stand-in.
void detachShaderNoop(GLuint program, GLuint shader) {
    (void) program;
    (void) shader;
}
} // namespace

VitaOpenGLContext::VitaOpenGLContext(int width, int height) {
    SceKernelFreeMemorySizeInfo freeBefore = {};
    freeBefore.size = sizeof(SceKernelFreeMemorySizeInfo);
    sceKernelGetFreeMemorySize(&freeBefore);
    MM_INFO("Free memory before vitaGL init: user {} KiB, cdram {} KiB, phycont {} KiB.",
            freeBefore.size_user / 1024, freeBefore.size_cdram / 1024, freeBefore.size_phycont / 1024);

    // The Vita panel is a fixed 960x544 - always hand vitaGL the native resolution instead of whatever the window
    // reports, so a 640x480 config can't end up as a small box in the panel's corner.
    constexpr int VITA_SCREEN_WIDTH = 960;
    constexpr int VITA_SCREEN_HEIGHT = 544;
    MM_INFO("Initializing vitaGL display at {}x{} (window reports {}x{}).",
            VITA_SCREEN_WIDTH, VITA_SCREEN_HEIGHT, width, height);

    // vitaGL hands itself (free memory - threshold) as its own pools, so the threshold is what's left to the engine.
    // Too small a threshold and vitaGL swallows (almost) all free user RAM; too large and vitaGL ends up with nothing
    // to work with. With the LODs streamed from disk (see LodReader::open(FileSystem *)) the engine doesn't need much,
    // so leave vitaGL the larger share.
    constexpr int RAM_RESERVED_FOR_ENGINE = 64 * 1024 * 1024;
    (void) vglInitExtended(0, VITA_SCREEN_WIDTH, VITA_SCREEN_HEIGHT, RAM_RESERVED_FOR_ENGINE, SCE_GXM_MULTISAMPLE_NONE);

    SceKernelFreeMemorySizeInfo freeAfter = {};
    freeAfter.size = sizeof(SceKernelFreeMemorySizeInfo);
    sceKernelGetFreeMemorySize(&freeAfter);
    MM_INFO("Free memory after vitaGL init: user {} KiB, cdram {} KiB, phycont {} KiB; vitaGL RAM {} KiB, VRAM {} KiB.",
            freeAfter.size_user / 1024, freeAfter.size_cdram / 1024, freeAfter.size_phycont / 1024,
            vglMemFree(VGL_MEM_RAM) / 1024, vglMemFree(VGL_MEM_VRAM) / 1024);

    _initialized = glGetString(GL_VERSION) != nullptr;
    if (!_initialized)
        MM_CRITICAL("vitaGL failed to initialize, no usable OpenGL context.");
}

VitaOpenGLContext::~VitaOpenGLContext() {
    // vitaGL has no teardown call - it owns the display until the process exits.
}

bool VitaOpenGLContext::bind() {
    return true; // vitaGL is a singleton, there's nothing to make current.
}

bool VitaOpenGLContext::unbind() {
    return true;
}

void *VitaOpenGLContext::nativeHandle() {
    return nullptr; // No EGL/GLX handle on Vita.
}

void VitaOpenGLContext::swapBuffers() {
    waitForResumeIfSuspended();
    vglSwapBuffers(GL_FALSE);
}

void *VitaOpenGLContext::getProcAddress(const char *name) {
    if (void *result = vglGetProcAddress(name))
        return result;

    // See detachShaderNoop above.
    if (std::strcmp(name, "glDetachShader") == 0)
        return reinterpret_cast<void *>(&detachShaderNoop);

    return nullptr;
}
