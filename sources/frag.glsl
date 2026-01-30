#version 430 core

in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
uniform bool uHasColor;

void main() {
    FragColor = vec4(uHasColor ? texture(uTexture, TexCoord).xyz : vec3(texture(uTexture, TexCoord).r), 1.0);
}