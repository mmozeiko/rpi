#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <time.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include "render.h"

///////////////////////////
const int ENABLE_VSYNC = 1;
///////////////////////////

struct drm
{
    int handle;

    struct gbm_device* device;
    struct gbm_surface* surface;
    struct gbm_bo* last_bo;

    uint32_t connector_id;
    drmModeCrtc* crtc;

    int waiting_for_flip;
};

struct egl
{
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
};

static void drm_init(struct drm* drm)
{
    drm->handle = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (drm->handle < 0)
    {
        assert(errno != ENOENT && "DRI device not found, have you enabled vc4 driver in /boot/config.txt?");
        assert(errno != EACCES && "no permission to open DRI device, is your user in 'video' group?");
        assert(!"cannot open DRI device");
    }

    drm->device = gbm_create_device(drm->handle);
    assert(drm->device && "cannot create GBM device");

    drmModeRes* resources = drmModeGetResources(drm->handle);
    assert(resources && "cannot get device resources");

    drmModeConnector* connector = NULL;
    for (int i=0; i<resources->count_connectors; i++)
    {
        connector = drmModeGetConnector(drm->handle, resources->connectors[i]);
        if (connector)
        {
            if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0)
            {
                drm->connector_id = connector->connector_id;
                break;
            }
            drmModeFreeConnector(connector);
            connector = NULL;
        }
    }
    assert(connector && "no display is connected");

    drmModeEncoder* encoder = NULL;
    for (int i=0; i<resources->count_encoders; i++)
    {
        encoder = drmModeGetEncoder(drm->handle, resources->encoders[i]);
        if (encoder)
        {
            if (encoder->encoder_id == connector->encoder_id)
            {
                break;
            }
            drmModeFreeEncoder(encoder);
            encoder = NULL;
        }
    }
    assert(encoder && "cannot find mode encoder");

    drm->crtc = drmModeGetCrtc(drm->handle, encoder->crtc_id);
    assert(drm->crtc && "cannot get current CRTC");

    drmModeFreeEncoder(encoder);
    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);

    uint32_t surface_format = GBM_FORMAT_XRGB8888;
    uint32_t surface_flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
    int supported = gbm_device_is_format_supported(drm->device, surface_format, surface_flags);
    assert(supported && "XRGB888 format is not supported for output rendering");

    uint32_t width = drm->crtc->mode.hdisplay;
    uint32_t height = drm->crtc->mode.vdisplay;

    drm->surface = gbm_surface_create(drm->device, width, height, surface_format, surface_flags);
    assert(drm->surface && "cannot create GBM output surface");

    drm->waiting_for_flip = 0;
    drm->last_bo = NULL;
}

static void drm_done(struct drm* drm)
{
    drmModeSetCrtc(drm->handle, drm->crtc->crtc_id, drm->crtc->buffer_id,
        drm->crtc->x, drm->crtc->y, &drm->connector_id, 1, &drm->crtc->mode);
    drmModeFreeCrtc(drm->crtc);

    if (drm->last_bo)
    {
        gbm_surface_release_buffer(drm->surface, drm->last_bo);
    }

    gbm_surface_destroy(drm->surface);
    gbm_device_destroy(drm->device);
    close(drm->handle);
}

static void egl_init(struct drm* drm, struct egl* egl)
{
    // assume EGL_EXT_client_extensions is available
    // printf("EGL client extension = %s\n", eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));

    // assume EGL_EXT_platform_base is available
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = (void*)eglGetProcAddress("eglGetPlatformDisplayEXT");
    PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC eglCreatePlatformWindowSurfaceEXT = (void*)eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");

    // assume EGL_MESA_platform_gbm is available
    egl->display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, drm->device, NULL);
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

    EGLConfig configs[256];
    EGLint config_count;
    ok = eglChooseConfig(egl->display, attribs, configs, sizeof(configs)/sizeof(*configs), &config_count);
    assert(ok && config_count != 0 && "cannot find suitable EGL configs");

    EGLConfig config = NULL;
    for (int i=0; i<config_count; i++)
    {
        EGLint format;
        if (eglGetConfigAttrib(egl->display, configs[i], EGL_NATIVE_VISUAL_ID, &format) && format == GBM_FORMAT_XRGB8888)
        {
            config = configs[i];
            break;
        }
    }
    assert(config && "cannot find EGL config that matches GBM surface format");

    egl->surface = eglCreatePlatformWindowSurfaceEXT(egl->display, config, drm->surface, NULL);
    assert(egl->surface != EGL_NO_SURFACE && "cannot create EGL surface");

    static const EGLint context_attrib[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,

        // use EGL_KHR_create_context if you want to debug context together with GL_KHR_debug
        // EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,

        // use EGL_KHR_create_context_no_error for "production" builds, may improve performance
        // EGL_CONTEXT_OPENGL_NO_ERROR_KHR, EGL_TRUE,

        EGL_NONE,
    };
    egl->context = eglCreateContext(egl->display, config, EGL_NO_CONTEXT, context_attrib);
    assert(egl->context != EGL_NO_CONTEXT && "cannot create EGL context");

    ok = eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context);
    assert(ok && "cannot set EGL context");
}

static void egl_done(struct egl* egl)
{
    eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(egl->display, egl->context);
    eglDestroySurface(egl->display, egl->surface);
    eglTerminate(egl->display);
}

static void output_destroy_bo(struct gbm_bo* bo, void* user)
{
    int dri = gbm_device_get_fd(gbm_bo_get_device(bo));
    uint32_t fb = (uint32_t)(uintptr_t)user;
    drmModeRmFB(dri, fb);
}

static uint32_t output_get_fb_from_bo(struct drm* drm, struct gbm_bo* bo)
{
    uint32_t fb = (uint32_t)(uintptr_t)gbm_bo_get_user_data(bo);
    if (fb)
    {
        return fb;
    }

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;

    int ok = drmModeAddFB(drm->handle, width, height, 24, 32, stride, handle, &fb);
    assert(ok == 0 && "cannot add DRM framebuffer");

    ok = drmModeSetCrtc(drm->handle, drm->crtc->crtc_id, fb, 0, 0, &drm->connector_id, 1, &drm->crtc->mode);
    assert(ok == 0 && "cannot set CRTC for DRM framebuffer");
    
    gbm_bo_set_user_data(bo, (void*)(uintptr_t)fb, output_destroy_bo);
    return fb;
}

static void output_page_flipped_handler(int fd, unsigned int sequence, unsigned int sec, unsigned int usec, void* user)
{
    int* waiting_for_flip = user;
    *waiting_for_flip = 0;
}

static int output_wait_for_flip(struct drm* drm, int timeout)
{
    drmEventContext ctx = {
        .version = DRM_EVENT_CONTEXT_VERSION,
        .page_flip_handler = output_page_flipped_handler,
    };

    while (drm->waiting_for_flip)
    {
        struct pollfd p = {
            .fd = drm->handle,
            .events = POLLIN,
        };
        if (poll(&p, 1, timeout) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            assert(!"poll error");
            return 0;
        }

        if (p.revents & POLLIN)
        {
            drmHandleEvent(drm->handle, &ctx);
        }
        else
        {
            // timeout reached
            return 0;
        }
    }
    return 1;
}

static void output_present(struct drm* drm, struct egl* egl)
{
    if (output_wait_for_flip(drm, ENABLE_VSYNC ? -1 : 0) == 0)
    {
        // frame is done faster than previous has finished flipping
        // so don't display it and continue with next frame
        return;
    }

    if (drm->last_bo)
    {
        gbm_surface_release_buffer(drm->surface, drm->last_bo);
    }

    EGLBoolean ok = eglSwapBuffers(egl->display, egl->surface);
    assert(ok && "cannot swap EGL buffers");

    struct gbm_bo* bo = gbm_surface_lock_front_buffer(drm->surface);
    assert(bo && "cannot lock GBM surface");
    drm->last_bo = bo;

    uint32_t fb = output_get_fb_from_bo(drm, bo);
    if (drmModePageFlip(drm->handle, drm->crtc->crtc_id, fb, DRM_MODE_PAGE_FLIP_EVENT, &drm->waiting_for_flip) == 0)
    {
        drm->waiting_for_flip = 1;
    }
}

static volatile int quit;

static void on_sigint(int dummy)
{
    quit = 1;
}

int main()
{
    struct drm drm;
    struct egl egl;

    printf("initialization\n");

    drm_init(&drm);
    egl_init(&drm, &egl);

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

        output_present(&drm, &egl);
    }

    printf("cleanup\n");

    egl_done(&egl);
    drm_done(&drm);

    printf("done\n");
}
