#pragma once

#include "Library/Platform/Interface/PlatformOpenGLContext.h"

/**
 * `PlatformOpenGLContext` implementation backed by vitaGL.
 *
 * The Vita video driver in SDL3 has no OpenGL support - SDL is shipped without the PVR backend, so
 * `SDL_CreateWindow(..., SDL_WINDOW_OPENGL)` fails with "OpenGL support is either not configured in SDL or not
 * available in current SDL video driver". vitaGL implements GLES2 on top of sceGxm, so on Vita we drive it
 * directly instead of going through SDL.
 */
class VitaOpenGLContext final : public PlatformOpenGLContext {
 public:
    VitaOpenGLContext(int width, int height);
    ~VitaOpenGLContext() override;

    bool bind() override;
    bool unbind() override;
    void *nativeHandle() override;
    void swapBuffers() override;

    void *getProcAddress(const char *name) override;

 private:
    bool _initialized = false;
};
