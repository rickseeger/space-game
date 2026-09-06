#version 330 core
in vec3 vColor;
uniform vec3 uColor;
uniform float uAlpha;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor * uColor, uAlpha);
}
