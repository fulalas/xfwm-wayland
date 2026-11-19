#include <EGL/egl.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>

typedef EGLBoolean (*make_current_function)(EGLDisplay display, EGLSurface draw, EGLSurface read,
                                            EGLContext context);

static bool is_current(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context);

EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context)
{
    static make_current_function library_make_current;

    if (is_current(display, draw, read, context)) {
        return EGL_TRUE;
    }
    if (library_make_current == NULL) {
        library_make_current = (make_current_function) dlsym(RTLD_NEXT, "eglMakeCurrent");
    }
    if (library_make_current == NULL) {
        return EGL_FALSE;
    }
    return library_make_current(display, draw, read, context);
}

static bool is_current(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context)
{
    return context != EGL_NO_CONTEXT && eglGetCurrentContext() == context &&
           eglGetCurrentDisplay() == display && eglGetCurrentSurface(EGL_DRAW) == draw &&
           eglGetCurrentSurface(EGL_READ) == read;
}
