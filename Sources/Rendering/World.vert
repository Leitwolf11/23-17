// SPDX-License-Identifier: MIT
#version 420 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec4 tangent;
layout(location=3) in vec3 tint;
layout(location=4) in vec3 coordinates;
layout(location=5) in vec4 visibility;
layout(location=6) in vec4 response;
#ifdef OPENGL_SCENE
layout(std140, binding=0)
#else
layout(std140, set=1, binding=0)
#endif
uniform Camera {
    vec4 eye, right, up, forward;
    vec4 lamps[4], spectra[4];
    vec4 probeCenter, probeHalfSize;
    vec4 streetCenter, streetHalfSize, streetLamp;
};
layout(location=0) out vec2 uv;
layout(location=1) flat out int layer;
layout(location=2) out vec3 surfaceTint;
layout(location=3) out vec4 lightVisibility;
layout(location=4) out vec3 worldPosition;
layout(location=5) out vec3 worldNormal;
layout(location=6) out vec4 worldTangent;
layout(location=7) out vec2 ambientEmission;
layout(location=8) out float fogAmount;
void main()
{
    vec3 posed=position;
    posed.x+=response.w*0.008*sin(1.20*probeCenter.w+position.x);
    vec3 relative = posed - eye.xyz;
    float z = dot(relative, forward.xyz);
    const float nearPlane = 0.06, farPlane = 46.0;
    gl_Position = vec4(dot(relative, right.xyz)*right.w, dot(relative, up.xyz)*up.w,
                       (z-nearPlane)*farPlane/(farPlane-nearPlane), z);
#ifdef OPENGL_SCENE
    gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;
#endif
    worldNormal=normal;
    worldTangent=tangent;
    worldPosition=posed;
    lightVisibility=visibility;
    layer=forward.w>0.0 ? int(coordinates.z)-1 : -1;
    uv=coordinates.xy;
    surfaceTint=tint;
    ambientEmission=response.xy * mix(1.0, eye.w, response.z);
    fogAmount=1.0-exp(-0.022*max(z-2.0,0.0)*exp(-0.12*max(position.y,0.0)));
}
