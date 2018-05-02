#pragma once
//
//  Skybox.hpp
//  Project3_CSE167
//
//  Created by Brittany Zhang on 11/5/17.
//  Copyright © 2017 ManianaBonita. All rights reserved.
//

#ifndef Cube_h
#define Cube_h

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string> //

#include <Windows.h> //

#define GLFW_INCLUDE_GLEXT
#ifdef __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#else
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

using namespace std;

unsigned int skyboxTexture_left;
unsigned int skyboxTexture_right;
unsigned int skyboxTexture_room;
unsigned int cubemapTexture;

class Cube {
private:
	int size = 1;
	vector<string> myFaces;

public:
	bool isSkybox = false;
	bool isLeftEye = false;
	bool isRoom = false;

	glm::mat4 toWorld;

	GLuint VBO, VAO, EBO;
	GLuint uProjection, uModelview;

	Cube(int mySize, vector<string> faces, bool check, bool isLeft, bool room)
	{
		size = mySize;
		isSkybox = check;
		isLeftEye = isLeft;
		isRoom = room;

		//toWorld = glm::scale(glm::mat4(1.0f), glm::vec3((float)size));
		toWorld = glm::mat4(1.0f);

		myFaces = faces;

		if (check) {
			if (room) {
				skyboxTexture_room = loadCubemap(faces);
			}
			else if (isLeftEye) {
				skyboxTexture_left = loadCubemap(faces);
			}
			else {
				skyboxTexture_right = loadCubemap(faces);
			}
		}
		else {
			cubemapTexture = loadCubemap(faces);
		}

		// Create array object and buffers. Remember to delete your buffers when the object is destroyed!
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		// Bind the Vertex Array Object (VAO) first, then bind the associated buffers to it.
		// Consider the VAO as a container for all your buffers.
		glBindVertexArray(VAO);

		// Now bind a VBO to it as a GL_ARRAY_BUFFER. The GL_ARRAY_BUFFER is an array containing relevant data to what
		// you want to draw, such as vertices, normals, colors, etc.
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// glBufferData populates the most recently bound buffer with data starting at the 3rd argument and ending after
		// the 2nd argument number of indices. How does OpenGL know how long an index spans? Go to glVertexAttribPointer.
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// Enable the usage of layout location 0 (check the vertex shader to see what this is)
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,// This first parameter x should be the same as the number passed into the line "layout (location = x)" in the vertex shader. In this case, it's 0. Valid values are 0 to GL_MAX_UNIFORM_LOCATIONS.
			3, // This second line tells us how any components there are per vertex. In this case, it's 3 (we have an x, y, and z component)
			GL_FLOAT, // What type these components are
			GL_FALSE, // GL_TRUE means the values should be normalized. GL_FALSE means they shouldn't
			3 * sizeof(GLfloat), // Offset between consecutive indices. Since each of our vertices have 3 floats, they should have the size of 3 floats in between
			(GLvoid*)0); // Offset of the first vertex's component. In our case it's 0 since we don't pad the vertices array with anything.

						 // We've sent the vertex data over to OpenGL, but there's still something missing.
						 // In what order should it draw those vertices? That's why we'll need a GL_ELEMENT_ARRAY_BUFFER for this.
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		// Unbind the currently bound buffer so that we don't accidentally make unwanted changes to it.
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Unbind the VAO now so we don't accidentally tamper with it.
		// NOTE: You must NEVER unbind the element array buffer associated with a VAO!
		glBindVertexArray(0);
	};

	~Cube()
	{
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
	};


	unsigned int loadCubemap(vector<string> faces)
	{
		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		int width, height, nrChannels;
		for (unsigned int i = 0; i < faces.size(); i++)
		{
			unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
				);
				stbi_image_free(data);
			}
			else
			{
				cout << "Cubemap texture failed to load at path: " << faces[i] << endl;
				stbi_image_free(data);
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); // seamless edge

		return textureID;
	};

	void draw(GLuint shaderProgram, const glm::mat4 & projection, const glm::mat4 & modelview)
	{
		// If drawing skybox cull front face
		// otherwise cull back face

		glEnable(GL_CULL_FACE);
		GLuint uMode = glGetUniformLocation(shaderProgram, "tex_mode");

		// We need to calculate this because modern OpenGL does not keep track of any matrix other than the viewport (D)
		// Consequently, we need to forward the projection, view, and model matrices to the shader programs
		// Get the location of the uniform variables "projection" and "modelview"
		uProjection = glGetUniformLocation(shaderProgram, "projection");// light.colorvec3
																		//    uModelview = glGetUniformLocation(shaderProgram, "modelview");
		uModelview = glGetUniformLocation(shaderProgram, "view");

		// Now send these values to the shader program
		glUniformMatrix4fv(uProjection, 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(uModelview, 1, GL_FALSE, &modelview[0][0]);

		// skybox cube
		glBindVertexArray(VAO);

		if (isSkybox) {
			glCullFace(GL_FRONT);

			if (isRoom) {
				glUniform1i(uMode, 2); // texture mode 2

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture_room);
				glUniform1i(glGetUniformLocation(shaderProgram, "skyboxTex_room"), 2);
			}
			else if (isLeftEye) {
				glUniform1i(uMode, 0); // texture mode 0

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture_left);
				glUniform1i(glGetUniformLocation(shaderProgram, "skyboxTex_left"), 0);
			}
			else {
				glUniform1i(uMode, 1); // texture mode 1

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture_right);
				glUniform1i(glGetUniformLocation(shaderProgram, "skyboxTex_right"), 1);
			}
			
		}
		else {
			glCullFace(GL_BACK);
			glUniform1i(uMode, 3); // texture mode 3

			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
			glUniform1i(glGetUniformLocation(shaderProgram, "cubeTex"), 3);
		}

		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

		// Draw triangles
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		/*glDepthMask(GL_TRUE);*/

		glDepthFunc(GL_LESS); // set depth function back to default
	};


	/*void scale(GLuint shaderProgram, float val) {	
		glm::mat4 & temp = glm::scale(glm::mat4(1.0f), glm::vec3(val, val, val));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &temp[0][0]);
	}*/
	//

	// Define the coordinates and indices needed to draw the cube. Note that it is not necessary
	// to use a 2-dimensional array, since the layout in memory is the same as a 1-dimensional array.
	// This just looks nicer since it's easy to tell what coordinates/indices belong where.
	const GLfloat vertices[8][3] = {

	{ -(GLfloat)size, -(GLfloat)size,  (GLfloat)size },
	{ (GLfloat)size, -(GLfloat)size,  (GLfloat)size },
	{ (GLfloat)size,  (GLfloat)size,  (GLfloat)size },
	{ -(GLfloat)size,  (GLfloat)size,  (GLfloat)size },
	{ -(GLfloat)size, -(GLfloat)size, -(GLfloat)size },
	{ (GLfloat)size, -(GLfloat)size, -(GLfloat)size },
	{ (GLfloat)size,  (GLfloat)size, -(GLfloat)size },
	{ -(GLfloat)size, (GLfloat)size, -(GLfloat)size }

		 //"Front" vertices
		//{ -700.0, -700.0,  700.0 },{ 700.0, -700.0,  700.0 },{ 700.0,  700.0,  700.0 },{ -700.0,  700.0,  700.0 },
		
		 //"Back" vertices
	    //{ -700.0, -700.0, -700.0 },{ 700.0, -700.0, -700.0 },{ 700.0,  700.0, -700.0 },{ -700.0,  700.0, -700.0 }
	};

	// Note that GL_QUADS is deprecated in modern OpenGL (and removed from OSX systems).
	// This is why we need to draw each face as 2 triangles instead of 1 quadrilateral
	const GLuint indices[6][6] = {
		// Front face
		{ 0, 1, 2, 2, 3, 0 },
		// Top face
		{ 1, 5, 6, 6, 2, 1 },
		// Back face
		{ 7, 6, 5, 5, 4, 7 },
		// Bottom face
		{ 4, 0, 3, 3, 7, 4 },
		// Left face
		{ 4, 5, 1, 1, 0, 4 },
		// Right face
		{ 3, 2, 6, 6, 7, 3 }
	};
};

#endif

