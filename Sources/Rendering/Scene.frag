// SPDX-License-Identifier: MIT
#version 420 core
layout(location=0) in vec3 color;
layout(location=0) out vec4 outputColor;
void main() { outputColor = vec4(color, 1.0); }
