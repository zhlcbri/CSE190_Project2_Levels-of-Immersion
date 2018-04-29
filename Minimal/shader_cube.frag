#version 330 core

out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skyboxTex; // skybox texture
uniform samplerCube cubeTex; // cube texture

uniform int tex_mode; // 0 for skybox texture, 1 for cube texture

void main()
{
    // FragColor = vec4(0.4, 0.0, 0.0, 0.2);

	if (tex_mode == 0) {
	  FragColor = texture(skyboxTex, TexCoords);
	} 
	else {
	  FragColor = texture(cubeTex, TexCoords);
	}
}
