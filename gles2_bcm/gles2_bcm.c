#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <time.h>
#include <signal.h>

#include <bcm_host.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "../common/render.h"

///////////////////////////
const int ENABLE_VSYNC = 1;
///////////////////////////

struct bcm
{
    DISPMANX_DISPLAY_HANDLE_T display;
    DISPMANX_UPDATE_HANDLE_T update;
    DISPMANX_ELEMENT_HANDLE_T element;
    EGL_DISPMANX_WINDOW_T window;
};

struct egl
{
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
};

static void bcm_init(struct bcm* bcm)
{
    bcm_host_init();

    uint32_t screen = 0;

    // TODO: other intersting stuff to look at:
    // vc_tv_register_callback - registers callback to detect HDMI disconnect event
    // vc_tv_hdmi_get_supported_modes_new - gets supported HDMI output modes
    // vc_tv_hdmi_power_on_explicit_new - set specific HDMI output mode

    TV_DISPLAY_STATE_T state;
    int ok = vc_tv_get_display_state(&state);
    assert(ok == 0 && "cannot get current HDMI mode");

    uint32_t width = state.display.hdmi.width;
    uint32_t height = state.display.hdmi.height;

    // printf("HDMI output = %ux%u@%u\n", width, height, state.display.hdmi.frame_rate);

    VC_RECT_T dst = { 0, 0, width, height };
    VC_RECT_T src = { 0, 0, width << 16, height << 16 };

    bcm->display = vc_dispmanx_display_open(screen);
    assert(bcm->display && "cannot open display");

    bcm->update = vc_dispmanx_update_start(screen);
    assert(bcm->update && "cannot start display update");

    // disable blending on top of background terminal
    VC_DISPMANX_ALPHA_T alpha = { .flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, .opacity = 255 };

    bcm->window.element = vc_dispmanx_element_add(bcm->update, bcm->display, 0, &dst, 0, &src, DISPMANX_PROTECTION_NONE, &alpha, 0, 0);
    assert(bcm->window.element && "cannot add window element");
      
    bcm->window.width = width;
    bcm->window.height = height;

    ok = vc_dispmanx_update_submit_sync(bcm->update);
    assert(ok == 0 && "cannot submit update");
}

static void bcm_done(struct bcm* bcm)
{
    vc_dispmanx_element_remove(bcm->update, bcm->window.element);
    vc_dispmanx_display_close(bcm->display);
}

static void egl_init(struct bcm* bcm, struct egl* egl)
{
    // printf("EGL client extension = %s\n", eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));

    // if this function fails with "* failed to add service - already in use?" text on output
    // this probably means you have vc4 driver loaded in /boot/config.txt, remove it
    egl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(egl->display != EGL_NO_DISPLAY && "cannot get EGL display");

    EGLint major, minor;
    EGLBoolean ok = eglInitialize(egl->display, &major, &minor);
    assert(ok && "cannot initialize EGL display");

    // printf("EGL_VENDOR = %s\n", eglQueryString(display, EGL_VENDOR));
    // printf("EGL_VERSION = %s\n", eglQueryString(display, EGL_VERSION));
    // printf("EGL_CLIENT_APIS = %s\n", eglQueryString(display, EGL_CLIENT_APIS));
    // printf("EGL_EXTENSIONS = %s\n", eglQueryString(display, EGL_EXTENSIONS));

    ok = eglBindAPI(EGL_OPENGL_ES_API);
    assert(ok && "cannot use OpenGL ES API");

    static const EGLint attribs[] =
    {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        //EGL_DEPTH_SIZE, 24,
        //EGL_STENCIL_SIZE, 8,
        EGL_NONE,
    };

    EGLConfig config;
    EGLint config_count;
    ok = eglChooseConfig(egl->display, attribs, &config, 1, &config_count);
    assert(ok && config_count != 0 && "cannot find suitable EGL config");

    egl->surface = eglCreateWindowSurface(egl->display, config, &bcm->window, NULL);
    assert(egl->surface != EGL_NO_SURFACE && "cannot create EGL surface");

    static const EGLint context_attrib[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
    };
    egl->context = eglCreateContext(egl->display, config, EGL_NO_CONTEXT, context_attrib);
    assert(egl->context != EGL_NO_CONTEXT && "cannot create EGL context");

    ok = eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context);
    assert(ok && "cannot set EGL context");

    ok = eglSwapInterval(egl->display, ENABLE_VSYNC ? 1 : 0);
    assert(ok && "cannot set EGL vsync");
}

static void egl_done(struct egl* egl)
{
    eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(egl->display, egl->context);
    eglDestroySurface(egl->display, egl->surface);
    eglTerminate(egl->display);
}

static void output_present(struct egl* egl)
{
    EGLBoolean ok = eglSwapBuffers(egl->display, egl->surface);
    assert(ok && "cannot swap EGL buffers");
}

static volatile int quit;

static void on_sigint(int num)
{
    (void)num;
    quit = 1;
}

int main()
{
    struct bcm bcm;
    struct egl egl;

    printf("initialization\n");

    bcm_init(&bcm);
    egl_init(&bcm, &egl);

    // printf("GL_VENDOR = %s\n", glGetString(GL_VENDOR));
    // printf("GL_RENDERER = %s\n", glGetString(GL_RENDERER));
    // printf("GL_VERSION = %s\n", glGetString(GL_VERSION));
    // printf("GL_EXTENSIONS = %s\n", glGetString(GL_EXTENSIONS));

    printf("rendering...\n");

    signal(SIGINT, on_sigint);

    struct timespec begin, prev;
    clock_gettime(CLOCK_MONOTONIC, &begin);
    prev = begin;

    uint32_t frames = 0;

    while (!quit)
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        float msec = (now.tv_sec - begin.tv_sec) * 1e+3f + (now.tv_nsec - begin.tv_nsec) * 1e-6f;
        if (msec >= 1000.f)
        {
            printf("frame = %.2f msec, FPS = %.2f\n", msec / frames, frames * 1000.f / msec);
            fflush(stdout);
            begin = now;
            frames = 0;
        }
        frames++;

        float delta = (float)(now.tv_sec - prev.tv_sec) + (now.tv_nsec - prev.tv_nsec) * 1e-9f;
        render(delta);
        prev = now;

        output_present(&egl);
    }

    printf("cleanup\n");

    egl_done(&egl);
    bcm_done(&bcm);

    printf("done\n");
}
