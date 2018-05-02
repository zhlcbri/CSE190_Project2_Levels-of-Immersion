#version 330 core

out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skyboxTex_left; // skybox texture for left eye
uniform samplerCube skyboxTex_right; // skybox texture for right eye
uniform samplerCube skyboxTex_room; // room texture
uniform samplerCube cubeTex; // cube texture

// 0 for skybox texture (left eye), 1 for skybox texture (right eye)
// 2 for room texture, 3 for cube texture
uniform int tex_mode; 

void main()
{
    // FragColor = vec4(0.4, 0.0, 0.0, 0.2);

	if (tex_mode == 0) {
	  FragColor = texture(skyboxTex_left, TexCoords);
	} 
	else if (tex_mode == 1) {
	  FragColor = texture(skyboxTex_right, TexCoords);
	}
	else if (tex_mode == 2) {
	  FragColor = texture(skyboxTex_room, TexCoords);
	}
	else {
	  FragColor = texture(cubeTex, TexCoords);
	}
}
