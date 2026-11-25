// SPDX-License-Identifier: MIT
#version 420 core
layout(location = 0) in vec4 clipPosition;
layout(location = 1) in vec3 surfaceColor;
layout(location = 2) in vec3 materialCoordinates;
layout(location = 0) out vec3 color;
layout(location = 1) out vec2 texel;
layout(location = 2) flat out int material;

void main()
{
#ifdef OPENGL_SCENE
    gl_Position = vec4(clipPosition.xy, 2.0 * clipPosition.z - clipPosition.w, clipPosition.w);
#else
    gl_Position = clipPosition;
#endif
    color = surfaceColor;
    texel = materialCoordinates.xy;
    material = int(materialCoordinates.z);
}
