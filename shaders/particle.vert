#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aSize;
uniform mat4 uView;
uniform mat4 uProj;
out vec3 vColor;
void main() {
    vec4 viewPos = uView * vec4(aPos, 1.0);
    gl_Position = uProj * viewPos;
    float psz = aSize * (300.0 / max(abs(viewPos.z), 0.5));
    gl_PointSize = clamp(psz, 1.0, 64.0);
    vColor = aColor;
}
