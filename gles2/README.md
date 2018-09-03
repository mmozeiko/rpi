# gles2

Example how to do GLES2 rendering regardless of which OpenGL driver is loaded. It
will set up EGL context either with Broadcom or with vc4/mesa driver.

# Requirements

Check the readme files for `gles2_bcm` and `gles2_drm` demos in parent folder for
setting up required libraries and development header files.

# Compiling & running 

Execute `make` to build.

You can execute `make run` to build & upload & run executable on Raspberry Pi. By
default it will try to connect to pi@raspberrypi.local host. You can override it
by setting `PI` variable: `make run PI=alarm@alarmpi.local`

You should see colorful triangle spinning. By default vsync is enabled, but you
can disable it by changing last argument to `context_set_vsync` function.

# Using in your code

In case you are inegrating this code into your application, these are "public"
functions that you should use:

    void api_load(void);
    void api_unload(void);

    void context_init(struct context* ctx);
    void context_done(struct context* ctx);
    
    void context_present(struct context* ctx);
    void context_set_vsync(struct context* ctx, int vsync);
    void* context_get_address(const char* name);

Other functions are implementation details, do not call them directly.

Start with calling `api_load` function. It will load all necessary dynamically
libraries. You need to do this only once. If you want clean shutdown, then at
end of program call `api_unload` to unload dynamic libraries. Rest of `context_*`
functions can be called only after `api_load` was called and before
`api_unload` will be called.

Now for each OpenGL context you want to create call `context_init` function.
Then use `context_get_address` function to load GLES2 functions. Do not link
to system libGLESv2.so library, because depending on which backend is used,
the GLES2 library can be different file. Use `context_present` to swap back
buffer to screen. VSync can be controlled with `context_set_vsync` function,
use 0 to disable and 1 to enable vsync. To cleanly release OpenGL context,
call `context_done` function.

You probably will want to edit code a bit to handle errors more gracefully.
Currently all error conditions are handled with `assert`.
