#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube cubeTex; 

void main()
{
    FragColor = texture(cubeTex, TexCoords);
}