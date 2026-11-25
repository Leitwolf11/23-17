// SPDX-License-Identifier: MIT
#version 420 core
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outputColor;
#ifdef OPENGL_SCENE
layout(binding=0)
#else
layout(set=2,binding=0)
#endif
uniform sampler2D frameColor;
float luma(vec3 rgb) { return dot(rgb, vec3(0.299,0.587,0.114)); }
void main()
{
    vec2 pixel = 1.0 / vec2(textureSize(frameColor, 0));
    vec3 center = texture(frameColor,uv).rgb;
    float a = luma(texture(frameColor,uv + pixel*vec2(-0.5,-0.5)).rgb);
    float b = luma(texture(frameColor,uv + pixel*vec2(0.5,-0.5)).rgb);
    float c = luma(texture(frameColor,uv + pixel*vec2(-0.5,0.5)).rgb);
    float d = luma(texture(frameColor,uv + pixel*vec2(0.5,0.5)).rgb);
    float hi = max(max(a,b),max(c,d)), lo = min(min(a,b),min(c,d));
    if (hi-lo < max(0.035,hi*0.12)) { outputColor = vec4(center,1); return; }
    vec2 gradient = vec2(c+d-a-b,a+c-b-d);
    vec2 axis = clamp(gradient/(min(abs(gradient.x),abs(gradient.y))+0.04),-3.0,3.0)*pixel;
    vec3 smoothColor = 0.5*(texture(frameColor,uv-axis*0.25).rgb + texture(frameColor,uv+axis*0.25).rgb);
    outputColor = vec4(mix(center,smoothColor,0.75),1);
}
