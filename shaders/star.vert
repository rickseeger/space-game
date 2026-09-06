#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aBright;
uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uCamPos;
uniform float uSpeed;
uniform float uAspect;
out float vBright;
out float vStreak;
void main() {
    vec3 rel = aPos - uCamPos;
    // wrap into large cube around camera
    float wrap = 900.0;
    rel = mod(rel + wrap * 0.5, wrap) - wrap * 0.5;
    vec4 viewPos = uView * vec4(uCamPos + rel, 1.0);
    // streak along view-space velocity approximation (negative Z motion)
    float streak = clamp(uSpeed / 90.0, 0.0, 1.0);
    vStreak = streak;
    vBright = aBright;
    gl_Position = uProj * viewPos;
    float dist = length(rel);
    float size = mix(2.2, 1.0, clamp(dist / 400.0, 0.0, 1.0));
    size *= mix(1.0, 3.5, streak * aBright);
    gl_PointSize = size * (1200.0 / max(abs(viewPos.z), 1.0)) * 0.15;
    gl_PointSize = clamp(gl_PointSize, 1.2, 10.0 + streak * 18.0);
}
