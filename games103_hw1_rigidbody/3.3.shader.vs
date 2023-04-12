#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTextureCoord;

out vec3 aPosOut;
out vec3 ourColor;
out vec2 ourTexCoord;
uniform mat4 transform;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	aPosOut = aPos;
    ourColor = aColor;
	ourTexCoord = aTextureCoord;
}