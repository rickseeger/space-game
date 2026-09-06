#version 330 core
in vec3 vNormal;
in vec3 vColor;
in vec3 vWorldPos;
uniform vec3 uCamPos;
uniform vec3 uTint;
uniform float uGlow;
uniform float uFlash;
out vec4 FragColor;
void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 L1 = normalize(vec3(0.4, 0.8, 0.3));
    vec3 L2 = normalize(vec3(-0.5, 0.2, -0.7));
    float diff = max(dot(N, L1), 0.0) * 0.7 + max(dot(N, L2), 0.0) * 0.35;
    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.5);
    vec3 base = vColor * uTint;
    vec3 col = base * (0.18 + diff) + base * rim * 0.55;
    col += base * uGlow * 0.8;
    col += vec3(uFlash);
    // slight neon boost
    col = col * 1.15 + base * base * 0.2;
    FragColor = vec4(col, 1.0);
}
