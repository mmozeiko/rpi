#pragma once

// make sure you have GL defines and GL functions available globally to use this file

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

typedef struct
{
    float x, y;
    uint8_t r, g, b, a;
} vertex;

static const char* vertex_glsl =
    "attribute vec2 a_pos;              \n"
    "attribute vec4 a_color;            \n"
    "varying vec4 color;                \n"
    "                                   \n"
    "void main()                        \n"
    "{                                  \n"
    "  gl_Position = vec4(a_pos, 0, 1); \n"
    "  color = a_color;                 \n"
    "}                                  \n"
;

static const char* fragment_glsl =
    "#if GL_ES                \n"
    "precision lowp vec4;     \n"
    "#endif                   \n"
    "varying vec4 color;      \n"
    "                         \n"
    "void main()              \n"
    "{                        \n"
    "  gl_FragColor = color;  \n"
    "}                        \n"
;

static GLuint compile_shader(GLuint type, const char* glsl)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &glsl, NULL);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        char message[4096];
        glGetShaderInfoLog(shader, sizeof(message), NULL, message);
        printf("Error compiling %s shader!\n%s", type == GL_VERTEX_SHADER ? "vertex" : "fragment", message);
        assert(0);
    }
    return shader;
}

static void render(float delta)
{
    static int setup = 0;
    if (!setup)
    {
        setup = 1;

        GLuint vsh = compile_shader(GL_VERTEX_SHADER, vertex_glsl);
        GLuint fsh = compile_shader(GL_FRAGMENT_SHADER, fragment_glsl);

        GLuint program = glCreateProgram();
        glAttachShader(program, fsh);
        glAttachShader(program, vsh);
        glBindAttribLocation(program, 0, "a_pos");
        glBindAttribLocation(program, 1, "a_color");
        glLinkProgram(program);
        glDeleteShader(fsh);
        glDeleteShader(vsh);

        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char message[4096];
            glGetProgramInfoLog(program, sizeof(message), NULL, message);
            printf("Error linking shader!\n%s\n", message);
            assert(0);
        }

        glUseProgram(program);

        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, x));
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(vertex), (void*)offsetof(vertex, r));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glClearColor(0.392156863f, 0.584313725f, 0.929411765f, 1.f);
        assert(glGetError() == GL_NO_ERROR);
    }

    vertex vtx[] =
    {
        { .r = 255 },
        { .g = 255 },
        { .b = 255 },
    };

    static float t = 0.f;
    t = fmodf(t + delta, 2 * M_PI);

    for (int i=0; i<3; i++)
    {
        vtx[i].x = 0.8f * cosf(t + 2 * M_PI * i / 3) * 1080.f/1920.f;
        vtx[i].y = 0.8f * sinf(t + 2 * M_PI * i / 3);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx), vtx, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    assert(glGetError() == GL_NO_ERROR);
}
