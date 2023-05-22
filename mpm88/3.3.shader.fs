#version 330 core
out vec4 FragColor;

in vec3 aPos;
in vec3 ourColor;
in vec2 ourTexCoord;

uniform sampler2D ourTexture;

void main()
{
    FragColor = vec4(1.0, 1.0, 0.0, 1.0);
}