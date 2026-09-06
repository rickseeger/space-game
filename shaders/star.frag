#version 330 core
in float vBright;
in float vStreak;
out vec4 FragColor;
void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    // elongated along Y when streaking (approx)
    float stretch = 1.0 + vStreak * 4.0;
    uv.y *= stretch;
    float d = length(uv);
    if (d > 1.0) discard;
    float a = pow(1.0 - d, 1.5) * vBright;
    vec3 col = mix(vec3(0.7, 0.85, 1.0), vec3(1.0, 0.95, 0.85), vBright);
    col = mix(col, vec3(0.6, 0.9, 1.0), vStreak * 0.5);
    FragColor = vec4(col, a);
}
