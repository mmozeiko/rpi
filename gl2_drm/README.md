# gl2_drm

Example how to do GL2 rendering with open-source [vc4] OpenGL driver by using [libdrm][libdrm] and [mesa][mesa].


# Requirements

Check the readme files for [gles2_drm](../gles2_drn) demo in parent folder for setting up
required libraries and development header files.

Main differences from setting up GLES2 context:

* use `EGL_OPENGL_API` when calling `eglBindAPI`
* use `EGL_OPENGL_BIT` for `EGL_RENDERABLE_TYPE` when choosing config
* load GL functions with `eglGetProcAddress` because mesa does not have libGL.so for non-X11 builds,
  [more info][mesa-gl-no-x11]
* GLSL shader differences - no precision qualifiers in non-ES GLSL language (for GL 2.1 version)


# Building & running

Execute `make` to build.

You can execute `make run` to build & upload & run executable on Raspberry Pi. By default it will
try to connect to pi@raspberrypi.local host. You can override it by setting `PI` variable:
`make run PI=alarm@alarmpi.local`

You should see colorful triangle spinning. By default vsync is enabled, but you can disable it by changing
`ENABLE_VSYNC` variable in source code.

Error handling is done using `assert`, feel free to change it when porting to your code.

[vc4]: https://dri.freedesktop.org/docs/drm/gpu/vc4.html
[libdrm]: https://cgit.freedesktop.org/mesa/drm/
[mesa]: https://www.mesa3d.org/
[mesa-gl-no-x11]: https://lists.freedesktop.org/archives/mesa-users/2014-February/000770.html