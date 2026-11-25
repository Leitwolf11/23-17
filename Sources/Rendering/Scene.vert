// SPDX-License-Identifier: MIT
#version 420 core
layout(location=0) in vec4 clipPosition;
layout(location=1) in vec3 surfaceColor;
layout(location=0) out vec3 color;
void main()
{
    gl_Position = clipPosition;
#ifdef OPENGL_SCENE
    gl_Position.z = 2.0 * clipPosition.z - clipPosition.w;
#endif
    color = surfaceColor;
}
