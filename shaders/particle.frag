#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float d = length(uv);
    if (d > 1.0) discard;
    float a = pow(1.0 - d, 1.8);
    FragColor = vec4(vColor, a);
}
