#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <time.h>
#include <poll.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#define GL_GLES_PROTOTYPES 0 // this is important to prevent global GL function declarations
#include <GLES2/gl2.h>       // we use it only to access enum values & function pointer types (PFN...PROC)

// functions required for "render.h" to work
#define GL_FUNCTIONS \
    X( PFNGLCREATESHADERPROC,            glCreateShader            ) \
    X( PFNGLDELETESHADERPROC,            glDeleteShader            ) \
    X( PFNGLCREATEPROGRAMPROC,           glCreateProgram           ) \
    X( PFNGLSHADERSOURCEPROC,            glShaderSource            ) \
    X( PFNGLCOMPILESHADERPROC,           glCompileShader           ) \
    X( PFNGLATTACHSHADERPROC,            glAttachShader            ) \
    X( PFNGLLINKPROGRAMPROC,             glLinkProgram             ) \
    X( PFNGLGETSHADERIVPROC,             glGetShaderiv             ) \
    X( PFNGLGETPROGRAMIVPROC,            glGetProgramiv            ) \
    X( PFNGLGETSHADERINFOLOGPROC,        glGetShaderInfoLog        ) \
    X( PFNGLGETPROGRAMINFOLOGPROC,       glGetProgramInfoLog       ) \
    X( PFNGLBINDATTRIBLOCATIONPROC,      glBindAttribLocation      ) \
    X( PFNGLUSEPROGRAMPROC,              glUseProgram              ) \
    X( PFNGLGENBUFFERSPROC,              glGenBuffers              ) \
    X( PFNGLBINDBUFFERPROC,              glBindBuffer              ) \
    X( PFNGLBUFFERDATAPROC,              glBufferData              ) \
    X( PFNGLVERTEXATTRIBPOINTERPROC,     glVertexAttribPointer     ) \
    X( PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray ) \
    X( PFNGLDRAWARRAYSPROC,              glDrawArrays              ) \
    X( PFNGLCLEARCOLORPROC,              glClearColor              ) \
    X( PFNGLCLEARPROC,                   glClear                   ) \
    X( PFNGLGETERRORPROC,                glGetError                ) \
    X( PFNGLGETSTRINGPROC,               glGetString               ) \

#define X(type, name) static type name;
GL_FUNCTIONS
#undef X

#include "../common/render.h"

// these headers are below render.h include to make sure we are not using anything from them
#include <bcm_host.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define BCM_FUNCTIONS \
    X( void,                      bcm_host_init,                  (void)                                                           ) \
    X( void,                      bcm_host_deinit,                (void)                                                           ) \
    X( int,                       vc_tv_get_display_state,        (TV_DISPLAY_STATE_T*)                                            ) \
    X( DISPMANX_DISPLAY_HANDLE_T, vc_dispmanx_display_open,       (uint32_t)                                                       ) \
    X( DISPMANX_UPDATE_HANDLE_T,  vc_dispmanx_update_start,       (int32_t)                                                        ) \
    X( DISPMANX_ELEMENT_HANDLE_T, vc_dispmanx_element_add,        (DISPMANX_UPDATE_HANDLE_T, DISPMANX_DISPLAY_HANDLE_T, int32_t,     \
                                                                   const VC_RECT_T*, DISPMANX_RESOURCE_HANDLE_T, const VC_RECT_T*,   \
                                                                   DISPMANX_PROTECTION_T, VC_DISPMANX_ALPHA_T*, DISPMANX_CLAMP_T*,   \
                                                                   DISPMANX_TRANSFORM_T)                                           ) \
    X( int,                       vc_dispmanx_update_submit_sync, (DISPMANX_UPDATE_HANDLE_T)                                       ) \
    X( int,                       vc_dispmanx_element_remove,     (DISPMANX_UPDATE_HANDLE_T, DISPMANX_ELEMENT_HANDLE_T)            ) \
    X( int,                       vc_dispmanx_display_close,      (DISPMANX_DISPLAY_HANDLE_T)                                      ) \

#define DRM_FUNCTIONS \
    X( drmModeResPtr,       drmModeGetResources,  (int)                                                                             ) \
    X( drmModeConnectorPtr, drmModeGetConnector,  (int, uint32_t)                                                                   ) \
    X( drmModeEncoderPtr,   drmModeGetEncoder,    (int, uint32_t)                                                                   ) \
    X( drmModeCrtcPtr,      drmModeGetCrtc,       (int, uint32_t)                                                                   ) \
    X( int,                 drmModeSetCrtc,       (int, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t*, int, drmModeModeInfoPtr) ) \
    X( int,                 drmModeAddFB,         (int, uint32_t, uint32_t, uint8_t, uint8_t, uint32_t, uint32_t, uint32_t*)        ) \
    X( int,                 drmModeRmFB,          (int, uint32_t)                                                                   ) \
    X( int,                 drmModePageFlip,      (int, uint32_t, uint32_t, uint32_t, void*)                                        ) \
    X( int,                 drmHandleEvent,       (int, drmEventContextPtr)                                                         ) \
    X( void,                drmModeFreeCrtc,      (drmModeCrtcPtr)                                                                  ) \
    X( void,                drmModeFreeConnector, (drmModeConnectorPtr)                                                             ) \
    X( void,                drmModeFreeEncoder,   (drmModeEncoderPtr)                                                               ) \
    X( void,                drmModeFreeResources, (drmModeResPtr)                                                                   ) \

#define GBM_FUNCTIONS \
    X( struct gbm_device*,  gbm_create_device,              (int)                                                        ) \
    X( struct gbm_surface*, gbm_surface_create,             (struct gbm_device*, uint32_t, uint32_t, uint32_t, uint32_t) ) \
    X( int,                 gbm_device_is_format_supported, (struct gbm_device*, uint32_t, uint32_t)                     ) \
    X( int,                 gbm_device_get_fd,              (struct gbm_device*)                                         ) \
    X( void,                gbm_device_destroy,             (struct gbm_device*)                                         ) \
    X( void,                gbm_surface_destroy,            (struct gbm_surface*)                                        ) \
    X( struct gbm_bo*,      gbm_surface_lock_front_buffer,  (struct gbm_surface*)                                        ) \
    X( void,                gbm_surface_release_buffer,     (struct gbm_surface*, struct gbm_bo*)                        ) \
    X( struct gbm_device*,  gbm_bo_get_device,              (struct gbm_bo*)                                             ) \
    X( uint32_t,            gbm_bo_get_width,               (struct gbm_bo*)                                             ) \
    X( uint32_t,            gbm_bo_get_height,              (struct gbm_bo*)                                             ) \
    X( uint32_t,            gbm_bo_get_stride,              (struct gbm_bo*)                                             ) \
    X( union gbm_bo_handle, gbm_bo_get_handle,              (struct gbm_bo*)                                             ) \
    X( void,                gbm_bo_set_user_data,           (struct gbm_bo*, void*, void (*)(struct gbm_bo*, void*))     ) \
    X( void*,               gbm_bo_get_user_data,           (struct gbm_bo*)                                             ) \

#define EGL_FUNCTIONS \
    X( const char*, eglQueryString,         (EGLDisplay, EGLint)                                        ) \
    X( void*,       eglGetProcAddress,      (const char*)                                               ) \
    X( EGLDisplay,  eglGetDisplay,          (EGLNativeDisplayType)                                      ) \
    X( EGLBoolean,  eglInitialize,          (EGLDisplay, EGLint*, EGLint*)                              ) \
    X( EGLBoolean,  eglBindAPI,             (EGLenum)                                                   ) \
    X( EGLBoolean,  eglChooseConfig,        (EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*)    ) \
    X( EGLBoolean,  eglGetConfigAttrib,     (EGLDisplay, EGLConfig, EGLint, EGLint*)                    ) \
    X( EGLSurface,  eglCreateWindowSurface, (EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*) ) \
    X( EGLContext,  eglCreateContext,       (EGLDisplay, EGLConfig, EGLContext, const EGLint*)          ) \
    X( EGLBoolean,  eglMakeCurrent,         (EGLDisplay, EGLSurface, EGLSurface, EGLContext)            ) \
    X( EGLBoolean,  eglDestroyContext,      (EGLDisplay, EGLContext)                                    ) \
    X( EGLBoolean,  eglDestroySurface,      (EGLDisplay, EGLSurface)                                    ) \
    X( EGLBoolean,  eglTerminate,           (EGLDisplay)                                                ) \
    X( EGLBoolean,  eglSwapInterval,        (EGLDisplay, EGLint)                                        ) \
    X( EGLBoolean,  eglSwapBuffers,         (EGLDisplay, EGLSurface)                                    ) \

struct
{
    // shared library handles
    void* bcm;
    void* drm;
    void* gbm;
    void* egl;
    void* gles2;

#define X(ret, name, args) ret (EGLAPIENTRY *name) args;
EGL_FUNCTIONS
#undef X

    union
    {
        struct
        {
#define X(ret, name, args) ret (*name) args;
BCM_FUNCTIONS
#undef X
        };

        struct
        {
#define X(ret, name, args) ret (*name) args;
DRM_FUNCTIONS
GBM_FUNCTIONS
#undef X
        };
    };
}
static api; // storing loaded function pointers in api "namespace" to not conflict with global symbols

struct context
{
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;

    union
    {
        struct // drm & mesa backend 
        {
            int dri;

            struct gbm_device* gbm_device;
            struct gbm_surface* gbm_surface;
            struct gbm_bo* gbm_buffer;

            uint32_t drm_connector;
            drmModeCrtc* drm_crtc;

            int drm_waiting;
            int drm_vsync;
        };

        struct // bcm backend
        {
            DISPMANX_DISPLAY_HANDLE_T bcm_display;
            DISPMANX_UPDATE_HANDLE_T bcm_update;
            DISPMANX_ELEMENT_HANDLE_T bcm_element;

            struct EGL_DISPMANX_WINDOW_T
            {
                DISPMANX_ELEMENT_HANDLE_T element;
                int width;
                int height;
            } bcm_window;
        };
    };
};

static void context_set_vsync(struct context* ctx, int vsync)
{
    if (api.bcm)
    {
        EGLBoolean ok = api.eglSwapInterval(ctx->egl_display, vsync ? 1 : 0);
        assert(ok && "cannot set EGL vsync");
    }
    else
    {
        ctx->drm_vsync = vsync;
    }
}

static void api_load(void)
{
    const char* gles2;
    const char* egl;

    int bcm = (access("/sys/module/vc4/", F_OK) != 0);
    if (bcm)
    {
        api.bcm = dlopen("libbcm_host.so", RTLD_LAZY | RTLD_LOCAL);
        assert(api.bcm && "cannot load libbcm_host.so library");

#define X(ret, name, args) \
    api.name = dlsym(api.bcm, #name); \
    assert(api.name && "symbol not found in libbcm_host");
BCM_FUNCTIONS
#undef X
        api.bcm_host_init();

        gles2 = "libbrcmGLESv2.so";
        egl = "libbrcmEGL.so";
    }
    else
    {
        api.drm = dlopen("libdrm.so.2", RTLD_LAZY | RTLD_LOCAL);
        assert(api.drm && "cannot load libdrm.so.2 library");

        api.gbm = dlopen("libgbm.so.1", RTLD_LAZY | RTLD_LOCAL);
        assert(api.gbm && "cannot load libgbm.so.1 library");

#define X(ret, name, args) \
    api.name = dlsym(api.drm, #name); \
    assert(api.name && "symbol not found in libdrm");
DRM_FUNCTIONS
#undef X

#define X(ret, name, args) \
    api.name = dlsym(api.gbm, #name); \
    assert(api.name && "symbol not found in libgbm");
GBM_FUNCTIONS
#undef X
        gles2 = "libGLESv2.so.2";
        egl = "libEGL.so.1";
    }

    api.gles2 = dlopen(gles2, RTLD_LAZY | RTLD_LOCAL);
    assert(api.gles2 && "cannot load GLESv2 library");

    api.egl = dlopen(egl, RTLD_LAZY | RTLD_LOCAL);
    assert(api.egl && "cannot load EGL library");

#define X(ret, name, args) \
    api.name = dlsym(api.egl, #name); \
    assert(api.name && "cannot get EGL function");
EGL_FUNCTIONS
#undef X
}

static void api_unload(void)
{
    dlclose(api.egl);
    dlclose(api.gles2);

    if (api.bcm)
    {
        api.bcm_host_deinit();
        dlclose(api.bcm);
    }
    else
    {
        dlclose(api.gbm);
        dlclose(api.drm);
        
    }
    api.egl = api.gles2 = api.gbm = api.drm = api.bcm = NULL;
}

static void context_init(struct context* ctx)
{
    if (api.bcm)
    {
        TV_DISPLAY_STATE_T state;
        int ok = api.vc_tv_get_display_state(&state);
        assert(ok == VC_HDMI_SUCCESS && "cannot get current HDMI mode");

        // printf("HDMI ooutput = %ux%u@%u\n", state.display.hdmi.width, state.display.hdmi.height, state.display.hdmi.frame_rate);

        uint32_t width = state.display.hdmi.width;
        uint32_t height = state.display.hdmi.height;

        VC_RECT_T dst = { 0, 0, width, height };
        VC_RECT_T src = { 0, 0, width << 16, height << 16 };

        ctx->bcm_display = api.vc_dispmanx_display_open(DISPMANX_ID_MAIN_LCD);
        assert(ctx->bcm_display && "cannot open display");

        ctx->bcm_update = api.vc_dispmanx_update_start(DISPMANX_ID_MAIN_LCD);
        assert(ctx->bcm_update && "cannot start display update");

        // disable blending on top of background terminal
        VC_DISPMANX_ALPHA_T alpha = { .flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, .opacity = 255 };

        ctx->bcm_window.element = api.vc_dispmanx_element_add(ctx->bcm_update, ctx->bcm_display, 0, &dst, 0, &src, DISPMANX_PROTECTION_NONE, &alpha, 0, 0);
        assert(ctx->bcm_window.element && "cannot add window element");
      
        ctx->bcm_window.width = width;
        ctx->bcm_window.height = height;

        ok = api.vc_dispmanx_update_submit_sync(ctx->bcm_update);
        assert(ok == 0 && "cannot submit update");
    }
    else
    {
        ctx->dri = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
        if (ctx->dri < 0)
        {
            assert(errno != ENOENT && "DRI device not found, have you enabled vc4 driver in /boot/config.txt file?");
            assert(errno != EACCES && "no permission to open DRI device, is your user in 'video' group?");
            assert(!"cannot open DRI device");
        }

        ctx->gbm_device = api.gbm_create_device(ctx->dri);
        assert(ctx->gbm_device && "cannot create GBM device");

        drmModeRes* resources = api.drmModeGetResources(ctx->dri);
        assert(resources && "cannot get device resources");

        drmModeConnector* connector = NULL;
        for (int i=0; i<resources->count_connectors; i++)
        {
            connector = api.drmModeGetConnector(ctx->dri, resources->connectors[i]);
            if (connector)
            {
                if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0)
                {
                    ctx->drm_connector = connector->connector_id;
                    break;
                }
                api.drmModeFreeConnector(connector);
                connector = NULL;
            }
        }
        assert(connector && "no display is connected");

        drmModeEncoder* encoder = NULL;
        for (int i=0; i<resources->count_encoders; i++)
        {
            encoder = api.drmModeGetEncoder(ctx->dri, resources->encoders[i]);
            if (encoder)
            {
                if (encoder->encoder_id == connector->encoder_id)
                {
                    break;
                }
                api.drmModeFreeEncoder(encoder);
                encoder = NULL;
            }
        }
        assert(encoder && "cannot find mode encoder");

        ctx->drm_crtc = api.drmModeGetCrtc(ctx->dri, encoder->crtc_id);
        assert(ctx->drm_crtc && "cannot get current CRTC");

        // printf("Display mode = %ux%u@%u\n", drm->crtc->width, drm->crtc->height, drm->crtc->mode.vrefresh);

        api.drmModeFreeEncoder(encoder);
        api.drmModeFreeConnector(connector);
        api.drmModeFreeResources(resources);

        uint32_t surface_format = GBM_FORMAT_XRGB8888;
	uint32_t surface_flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
        int supported = api.gbm_device_is_format_supported(ctx->gbm_device, surface_format, surface_flags);
        assert(supported && "XRGB888 format is not supported for output rendering");

        uint32_t width = ctx->drm_crtc->mode.hdisplay;
        uint32_t height = ctx->drm_crtc->mode.vdisplay;

        ctx->gbm_surface = api.gbm_surface_create(ctx->gbm_device, width, height, surface_format, surface_flags);
        assert(ctx->gbm_surface && "cannot create GBM output surface");

        ctx->drm_waiting = 0;
        ctx->gbm_buffer = NULL;
    }

    const char* client_extension = api.eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    // printf("EGL client extenions = %s\n", client_extension);

    if (api.bcm)
    {
        ctx->egl_display = api.eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    else
    {
        // mesa on vc4 always supports these two extensions, but just to be 100% sure - let's check it
        assert(strstr(client_extension, "EGL_EXT_platform_base") && "EGL_EXT_platform_base client extension is required");
        assert(strstr(client_extension, "EGL_MESA_platform_gbm") && "EGL_MESA_platform_gbm client extension is required");

        PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = api.eglGetProcAddress("eglGetPlatformDisplayEXT");
        assert(eglGetPlatformDisplayEXT && "cannot get eglGetPlatformDisplayEXT extension function");

        ctx->egl_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, ctx->gbm_device, NULL);
    }
    assert(ctx->egl_display != EGL_NO_DISPLAY && "cannot get EGL display");

    EGLint major, minor;
    EGLBoolean ok = api.eglInitialize(ctx->egl_display, &major, &minor);
    assert(ok && "cannot initialize EGL display");

    // printf("EGL_VENDOR = %s\n", api.eglQueryString(ctx->egl_display, EGL_VENDOR));
    // printf("EGL_VERSION = %s\n", api.eglQueryString(ctx->egl_display, EGL_VERSION));
    // printf("EGL_CLIENT_APIS = %s\n", api.eglQueryString(ctx->egl_display, EGL_CLIENT_APIS));
    // printf("EGL_EXTENSIONS = %s\n", api.eglQueryString(ctx->egl_display, EGL_EXTENSIONS));

    ok = api.eglBindAPI(EGL_OPENGL_ES_API);
    assert(ok && "cannot use OpenGL ES API with EGL");

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

    if (api.bcm)
    {
        EGLint config_count;
        ok = api.eglChooseConfig(ctx->egl_display, attribs, &config, 1, &config_count);
        assert(ok && config_count != 0 && "cannot find suitable EGL config");

        ctx->egl_surface = api.eglCreateWindowSurface(ctx->egl_display, config, &ctx->bcm_window, NULL);
    }
    else
    {
        EGLConfig configs[32];
        EGLint config_count;
        ok = api.eglChooseConfig(ctx->egl_display, attribs, configs, sizeof(configs)/sizeof(*configs), &config_count);
        assert(ok && config_count != 0 && "cannot find suitable EGL configs");

        config = NULL;
        for (int i=0; i<config_count; i++)
        {
            EGLint format;
            if (api.eglGetConfigAttrib(ctx->egl_display, configs[i], EGL_NATIVE_VISUAL_ID, &format) && format == GBM_FORMAT_XRGB8888)
            {
                config = configs[i];
                break;
            }
        }
        assert(config && "cannot find EGL config that matches GBM surface format");

        PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC eglCreatePlatformWindowSurfaceEXT = api.eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");
        assert(eglCreatePlatformWindowSurfaceEXT && "cannot get eglCreatePlatformWindowSurfaceEXT extension function");

        ctx->egl_surface = eglCreatePlatformWindowSurfaceEXT(ctx->egl_display, config, ctx->gbm_surface, NULL);
    }
    assert(ctx->egl_surface != EGL_NO_SURFACE && "cannot create EGL surface");

    static const EGLint context_attrib[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,

        // use EGL_KHR_create_context if you want to debug context together with GL_KHR_debug
        // EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,

        // use EGL_KHR_create_context_no_error for "production" builds, may improve performance
        // EGL_CONTEXT_OPENGL_NO_ERROR_KHR, EGL_TRUE,

        EGL_NONE,
    };
    ctx->egl_context = api.eglCreateContext(ctx->egl_display, config, EGL_NO_CONTEXT, context_attrib);
    assert(ctx->egl_context != EGL_NO_CONTEXT && "cannot create EGL context");

    ok = api.eglMakeCurrent(ctx->egl_display, ctx->egl_surface, ctx->egl_surface, ctx->egl_context);
    assert(ok && "cannot set EGL context");    
}

static void context_done(struct context* ctx)
{
    api.eglMakeCurrent(ctx->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    api.eglDestroyContext(ctx->egl_display, ctx->egl_context);
    api.eglDestroySurface(ctx->egl_display, ctx->egl_surface);
    api.eglTerminate(ctx->egl_display);

    if (api.bcm)
    {
        api.vc_dispmanx_element_remove(ctx->bcm_update, ctx->bcm_window.element);
        api.vc_dispmanx_display_close(ctx->bcm_display);
    }
    else
    {
        api.drmModeSetCrtc(ctx->dri, ctx->drm_crtc->crtc_id, ctx->drm_crtc->buffer_id,
            ctx->drm_crtc->x, ctx->drm_crtc->y, &ctx->drm_connector, 1, &ctx->drm_crtc->mode);
        api.drmModeFreeCrtc(ctx->drm_crtc);

        if (ctx->gbm_buffer)
        {
            api.gbm_surface_release_buffer(ctx->gbm_surface, ctx->gbm_buffer);
        }

        api.gbm_surface_destroy(ctx->gbm_surface);
        api.gbm_device_destroy(ctx->gbm_device);
        close(ctx->dri);
    }
}

static void context_drm_destroy_buffer(struct gbm_bo* buffer, void* user)
{
    int dri = api.gbm_device_get_fd(api.gbm_bo_get_device(buffer));
    uint32_t fb = (uint32_t)(uintptr_t)user;
    api.drmModeRmFB(dri, fb);
}

static uint32_t output_drm_get_fb_from_buffer(struct context* ctx, struct gbm_bo* buffer)
{
    uint32_t fb = (uint32_t)(uintptr_t)api.gbm_bo_get_user_data(buffer);
    if (fb)
    {
        return fb;
    }

    uint32_t width = api.gbm_bo_get_width(buffer);
    uint32_t height = api.gbm_bo_get_height(buffer);
    uint32_t stride = api.gbm_bo_get_stride(buffer);
    uint32_t handle = api.gbm_bo_get_handle(buffer).u32;

    int ok = api.drmModeAddFB(ctx->dri, width, height, 24, 32, stride, handle, &fb);
    assert(ok == 0 && "cannot add DRM framebuffer");

    ok = api.drmModeSetCrtc(ctx->dri, ctx->drm_crtc->crtc_id, fb, 0, 0, &ctx->drm_connector, 1, &ctx->drm_crtc->mode);
    assert(ok == 0 && "cannot set CRTC for DRM framebuffer");

    api.gbm_bo_set_user_data(buffer, (void*)(uintptr_t)fb, context_drm_destroy_buffer);
    return fb;
}

static void output_drm_page_flip_handler(int fd, uint32_t sequence, uint32_t sec, uint32_t usec, void* user)
{
    (void)fd;
    (void)sequence;
    (void)sec;
    (void)usec;

    struct context* ctx = user;
    ctx->drm_waiting = 0;
}

static int context_drm_wait_for_flip(struct context* ctx)
{
    while (ctx->drm_waiting)
    {
        struct pollfd p = {
            .fd = ctx->dri,
            .events = POLLIN,
        };
        if (poll(&p, 1, ctx->drm_vsync ? -1 : 0) < 0)
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
            drmEventContext event_ctx = {
                .version = DRM_EVENT_CONTEXT_VERSION,
                .page_flip_handler = output_drm_page_flip_handler,
            };
            api.drmHandleEvent(ctx->dri, &event_ctx);
        }
        else
        {
            // timeout reached, this means the page flip did not happen
            return 0;
        }
    }
    return 1;
}

static void context_present(struct context* ctx)
{
    if (api.bcm)
    {
        EGLBoolean ok = api.eglSwapBuffers(ctx->egl_display, ctx->egl_surface);
        assert(ok && "cannot swap EGL buffers");
    }
    else
    {
        if (context_drm_wait_for_flip(ctx) == 0)
        {
            // frame is done faster than previous has finished flipping
            // so don't display it and continue with rendering of next frame
            return;
        }

        if (ctx->gbm_buffer)
        {
            api.gbm_surface_release_buffer(ctx->gbm_surface, ctx->gbm_buffer);
        }

        EGLBoolean ok = api.eglSwapBuffers(ctx->egl_display, ctx->egl_surface);
        assert(ok && "cannot swap EGL buffers");

        struct gbm_bo* buffer = api.gbm_surface_lock_front_buffer(ctx->gbm_surface);
        assert(buffer && "cannot lock GBM surface");
        ctx->gbm_buffer = buffer;

        uint32_t fb = output_drm_get_fb_from_buffer(ctx, buffer);
        if (api.drmModePageFlip(ctx->dri, ctx->drm_crtc->crtc_id, fb, DRM_MODE_PAGE_FLIP_EVENT, ctx) == 0)
        {
            ctx->drm_waiting = 1;
        }
    }
}

static void* context_get_address(const char* name)
{
    void* fn = api.eglGetProcAddress(name);
    // in case EGL doesn't want to return GLES2 spec function by name
    if (!fn) fn = dlsym(api.gles2, name);
    return fn;
}

static volatile int quit;

static void on_sigint(int num)
{
    (void)num;
    quit = 1;
}

int main()
{
    struct context ctx;

    printf("initialization\n");

    api_load();
    context_init(&ctx);
    context_set_vsync(&ctx, 1);

#define X(type, name) \
    name = context_get_address(#name); \
    assert(name && "cannot get GLES2 function");
GL_FUNCTIONS
#undef X

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

        context_present(&ctx);
    }

    printf("cleanup\n");

    context_done(&ctx);
    api_unload();

    printf("done\n");
}
