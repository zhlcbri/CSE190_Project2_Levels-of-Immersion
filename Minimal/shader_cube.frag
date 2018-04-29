#version 330 core

out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube cubeTex; 

void main()
{
    // FragColor = vec4(0.4, 0.0, 0.0, 0.2);

    FragColor = texture(cubeTex, TexCoords);
}
