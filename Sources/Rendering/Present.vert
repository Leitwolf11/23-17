// SPDX-License-Identifier: MIT
#version 420 core
layout(location=0) out vec2 uv;
void main()
{
#ifdef OPENGL_SCENE
    int vertex = gl_VertexID;
#else
    int vertex = gl_VertexIndex;
#endif
    uv = vec2((vertex << 1) & 2, vertex & 2);
    gl_Position = vec4(uv * 2.0 - 1.0, 0, 1);
#ifndef OPENGL_SCENE
    uv.y = 1.0 - uv.y;
#endif
}
