/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Brad Davis. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/

#include "Cube.h"
#include "shader.h"

#include <iostream>
#include <memory>
#include <exception>
#include <algorithm>
#include <Windows.h>
#include <math.h>

//#include <limits>

#define _CRT_SECURE_NO_WARNINGS //
#define __STDC_FORMAT_MACROS 1

#define FAIL(X) throw std::runtime_error(X)

///////////////////////////////////////////////////////////////////////////////
//
// GLM is a C++ math library meant to mirror the syntax of GLSL 
//

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>


// Import the most commonly used types into the default namespace
using glm::ivec3;
using glm::ivec2;
using glm::uvec2;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::quat;

using namespace std;

///////////////////////////////////////////////////////////////////////////////
//
// GLEW gives cross platform access to OpenGL 3.x+ functionality.  
//

#include <GL/glew.h>

/////// Custom variables
bool cube_size_up = false; // set to true with LThumbStick to right
bool cube_size_down = false; // set to true with LThumbStick to left
bool cube_size_reset = false; // set to true with LThumbStick pressed in

double iod = 0.0;
double original_iod = 0.0;
bool iod_up = false; // true when RThumbStick to right
bool iod_down = false; // true when RThumbStick to left
bool iod_reset = false; // true when RThumbStick pressed in

// Button X controls
bool x1 = true; // show entire scene (cubes and sky box in stereo)
bool x2 = false; // show just the sky box in stereo
bool x3 = false; // show just the sky box in mono
bool x4 = false; // show room instead of bear skybox

// Button A controls
bool a1 = true; // 3D stereo
bool a2 = false; // mono (the same image rendered on both eyes)
bool a3 = false; // left eye only (right eye black)
bool a4 = false; // right eye only (left eye black)
bool a5 = false; // inverted stereo (left eye image rendered to right eye and vice versa)

// Button B controls
bool b1 = true; // regular head tracking (both position and orientation)
bool b2 = false; // orientation only (position frozen to what it just was before the mode was selected)
bool b3 = false; // position only (orientation frozen to what it just was)
bool b4 = false; // no tracking (position and orientation frozen to what they just were when the user pressed the button)

// Button Y controls
bool superRotation = false; // toggled by Y button

bool isPressed = false; // true if any button is pressed

// HMD transformation matrices
glm::mat4 headPos_left_curr = glm::mat4(1.0f);
glm::mat4 headPos_right_curr = glm::mat4(1.0f);
glm::mat4 headPos_left_prev = glm::mat4(1.0f); // use this matrix to track head position each frame
glm::mat4 headPos_right_prev = glm::mat4(1.0f);

// euler angles of HMD's rotation matrix

float theta_x_curr, theta_y_curr, theta_z_curr = 0.0f;
float theta_x_prev, theta_y_prev, theta_z_prev = 0.0f;
glm::mat4 headPos_curr = glm::mat4(1.0f);
glm::mat4 headPos_prev = glm::mat4(1.0f);

float pi = atanf(1) * 4;

//////////////////////

bool checkFramebufferStatus(GLenum target = GL_FRAMEBUFFER) {
	GLuint status = glCheckFramebufferStatus(target);
	switch (status) {
	case GL_FRAMEBUFFER_COMPLETE:
		return true;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		std::cerr << "framebuffer incomplete attachment" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		std::cerr << "framebuffer missing attachment" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		std::cerr << "framebuffer incomplete draw buffer" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		std::cerr << "framebuffer incomplete read buffer" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		std::cerr << "framebuffer incomplete multisample" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		std::cerr << "framebuffer incomplete layer targets" << std::endl;
		break;

	case GL_FRAMEBUFFER_UNSUPPORTED:
		std::cerr << "framebuffer unsupported internal format or image" << std::endl;
		break;

	default:
		std::cerr << "other framebuffer error" << std::endl;
		break;
	}

	return false;
}

bool checkGlError() {
	GLenum error = glGetError();
	if (!error) {
		return false;
	}
	else {
		switch (error) {
		case GL_INVALID_ENUM:
			std::cerr << ": An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.";
			break;
		case GL_INVALID_VALUE:
			std::cerr << ": A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag";
			break;
		case GL_INVALID_OPERATION:
			std::cerr << ": The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag..";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			std::cerr << ": The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.";
			break;
		case GL_OUT_OF_MEMORY:
			std::cerr << ": There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
			break;
		case GL_STACK_UNDERFLOW:
			std::cerr << ": An attempt has been made to perform an operation that would cause an internal stack to underflow.";
			break;
		case GL_STACK_OVERFLOW:
			std::cerr << ": An attempt has been made to perform an operation that would cause an internal stack to overflow.";
			break;
		}
		return true;
	}
}

void glDebugCallbackHandler(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, GLvoid* data) {
	OutputDebugStringA(msg);
	std::cout << "debug call: " << msg << std::endl;
}

//////////////////////////////////////////////////////////////////////
//
// GLFW provides cross platform window creation
//

#include <GLFW/glfw3.h>

namespace glfw {
	inline GLFWwindow * createWindow(const uvec2 & size, const ivec2 & position = ivec2(INT_MIN)) {
		GLFWwindow * window = glfwCreateWindow(size.x, size.y, "glfw", nullptr, nullptr);
		if (!window) {
			FAIL("Unable to create rendering window");
		}
		if ((position.x > INT_MIN) && (position.y > INT_MIN)) {
			glfwSetWindowPos(window, position.x, position.y);
		}
		return window;
	}
}

// A class to encapsulate using GLFW to handle input and render a scene
class GlfwApp {

protected:
	uvec2 windowSize;
	ivec2 windowPosition;
	GLFWwindow * window{ nullptr };
	unsigned int frame{ 0 };

public:
	GlfwApp() {
		// Initialize the GLFW system for creating and positioning windows
		if (!glfwInit()) {
			FAIL("Failed to initialize GLFW");
		}
		glfwSetErrorCallback(ErrorCallback);
	}

	virtual ~GlfwApp() {
		if (nullptr != window) {
			glfwDestroyWindow(window);
		}
		glfwTerminate();
	}

	virtual int run() {
		preCreate();

		window = createRenderingTarget(windowSize, windowPosition);

		if (!window) {
			std::cout << "Unable to create OpenGL window" << std::endl;
			return -1;
		}

		postCreate();

		initGl();

		while (!glfwWindowShouldClose(window)) {
			++frame;
			glfwPollEvents();
			update();
			draw();
			finishFrame();
		}

		shutdownGl();

		return 0;
	}


protected:
	virtual GLFWwindow * createRenderingTarget(uvec2 & size, ivec2 & pos) = 0;

	virtual void draw() = 0;

	void preCreate() {
		glfwWindowHint(GLFW_DEPTH_BITS, 16);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	}


	void postCreate() {
		glfwSetWindowUserPointer(window, this);
		glfwSetKeyCallback(window, KeyCallback);
		glfwSetMouseButtonCallback(window, MouseButtonCallback);
		glfwMakeContextCurrent(window);

		// Initialize the OpenGL bindings
		// For some reason we have to set this experminetal flag to properly
		// init GLEW if we use a core context.
		glewExperimental = GL_TRUE;
		if (0 != glewInit()) {
			FAIL("Failed to initialize GLEW");
		}
		glGetError();

		if (GLEW_KHR_debug) {
			GLint v;
			glGetIntegerv(GL_CONTEXT_FLAGS, &v);
			if (v & GL_CONTEXT_FLAG_DEBUG_BIT) {
				//glDebugMessageCallback(glDebugCallbackHandler, this);
			}
		}
	}

	virtual void initGl() {
	}

	virtual void shutdownGl() {
	}

	virtual void finishFrame() {
		glfwSwapBuffers(window);
	}

	virtual void destroyWindow() {
		glfwSetKeyCallback(window, nullptr);
		glfwSetMouseButtonCallback(window, nullptr);
		glfwDestroyWindow(window);
	}

	virtual void onKey(int key, int scancode, int action, int mods) {
		if (GLFW_PRESS != action) {
			return;
		}

		switch (key) {
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, 1);
			return;
		}
	}

	virtual void update() {}

	virtual void onMouseButton(int button, int action, int mods) {}

protected:
	virtual void viewport(const ivec2 & pos, const uvec2 & size) {
		glViewport(pos.x, pos.y, size.x, size.y);
	}

private:

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		GlfwApp * instance = (GlfwApp *)glfwGetWindowUserPointer(window);
		instance->onKey(key, scancode, action, mods);
	}

	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		GlfwApp * instance = (GlfwApp *)glfwGetWindowUserPointer(window);
		instance->onMouseButton(button, action, mods);
	}

	static void ErrorCallback(int error, const char* description) {
		FAIL(description);
	}
};

//////////////////////////////////////////////////////////////////////
//
// The Oculus VR C API provides access to information about the HMD
//

#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

namespace ovr {

	// Convenience method for looping over each eye with a lambda
	template <typename Function>
	inline void for_each_eye(Function function) {
		for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
			eye < ovrEyeType::ovrEye_Count;
			eye = static_cast<ovrEyeType>(eye + 1)) {
			function(eye);
		}
	}

	inline mat4 toGlm(const ovrMatrix4f & om) {
		return glm::transpose(glm::make_mat4(&om.M[0][0]));
	}

	inline mat4 toGlm(const ovrFovPort & fovport, float nearPlane = 0.01f, float farPlane = 10000.0f) {
		return toGlm(ovrMatrix4f_Projection(fovport, nearPlane, farPlane, true));
	}

	inline vec3 toGlm(const ovrVector3f & ov) {
		return glm::make_vec3(&ov.x);
	}

	inline vec2 toGlm(const ovrVector2f & ov) {
		return glm::make_vec2(&ov.x);
	}

	inline uvec2 toGlm(const ovrSizei & ov) {
		return uvec2(ov.w, ov.h);
	}

	inline quat toGlm(const ovrQuatf & oq) {
		return glm::make_quat(&oq.x);
	}

	inline mat4 toGlm(const ovrPosef & op) {
		mat4 orientation = glm::mat4_cast(toGlm(op.Orientation));
		mat4 translation = glm::translate(mat4(), ovr::toGlm(op.Position));
		return translation * orientation;
	}

	inline ovrMatrix4f fromGlm(const mat4 & m) {
		ovrMatrix4f result;
		mat4 transposed(glm::transpose(m));
		memcpy(result.M, &(transposed[0][0]), sizeof(float) * 16);
		return result;
	}

	inline ovrVector3f fromGlm(const vec3 & v) {
		ovrVector3f result;
		result.x = v.x;
		result.y = v.y;
		result.z = v.z;
		return result;
	}

	inline ovrVector2f fromGlm(const vec2 & v) {
		ovrVector2f result;
		result.x = v.x;
		result.y = v.y;
		return result;
	}

	inline ovrSizei fromGlm(const uvec2 & v) {
		ovrSizei result;
		result.w = v.x;
		result.h = v.y;
		return result;
	}

	inline ovrQuatf fromGlm(const quat & q) {
		ovrQuatf result;
		result.x = q.x;
		result.y = q.y;
		result.z = q.z;
		result.w = q.w;
		return result;
	}
}

class RiftManagerApp {
protected:
	ovrSession _session;
	ovrHmdDesc _hmdDesc;
	ovrGraphicsLuid _luid;

public:
	RiftManagerApp() {
		if (!OVR_SUCCESS(ovr_Create(&_session, &_luid))) {
			FAIL("Unable to create HMD session");
		}

		_hmdDesc = ovr_GetHmdDesc(_session);
	}

	~RiftManagerApp() {
		ovr_Destroy(_session);
		_session = nullptr;
	}
};

class RiftApp : public GlfwApp, public RiftManagerApp {
public:

private:
	GLuint _fbo{ 0 };
	GLuint _depthBuffer{ 0 };
	ovrTextureSwapChain _eyeTexture;

	GLuint _mirrorFbo{ 0 };
	ovrMirrorTexture _mirrorTexture;

	ovrEyeRenderDesc _eyeRenderDescs[2]; // ?

	mat4 _eyeProjections[2];

	ovrLayerEyeFov _sceneLayer;
	ovrViewScaleDesc _viewScaleDesc;

	uvec2 _renderTargetSize;
	uvec2 _mirrorSize;

public:

	RiftApp() {
		using namespace ovr;
		_viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;

		memset(&_sceneLayer, 0, sizeof(ovrLayerEyeFov));
		_sceneLayer.Header.Type = ovrLayerType_EyeFov;
		_sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;

		ovr::for_each_eye([&](ovrEyeType eye) {
			ovrEyeRenderDesc& erd = _eyeRenderDescs[eye] = ovr_GetRenderDesc(_session, eye, _hmdDesc.DefaultEyeFov[eye]);
			ovrMatrix4f ovrPerspectiveProjection =
				ovrMatrix4f_Projection(erd.Fov, 0.01f, 1000.0f, ovrProjection_ClipRangeOpenGL);
			_eyeProjections[eye] = ovr::toGlm(ovrPerspectiveProjection);

			// cse190: adjust the eye separation here - need to use 3D vector from central point on Rift for each eye
			_viewScaleDesc.HmdToEyePose[eye] = erd.HmdToEyePose; 

			// get IOD see slides
		    iod = abs(_viewScaleDesc.HmdToEyePose[0].Position.x - _viewScaleDesc.HmdToEyePose[1].Position.x);
			original_iod = abs(_viewScaleDesc.HmdToEyePose[0].Position.x - _viewScaleDesc.HmdToEyePose[1].Position.x);
			
			//cout << "original iod is: " << original_iod << endl;

			ovrFovPort & fov = _sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
			auto eyeSize = ovr_GetFovTextureSize(_session, eye, fov, 1.0f);
			_sceneLayer.Viewport[eye].Size = eyeSize;
			_sceneLayer.Viewport[eye].Pos = { (int)_renderTargetSize.x, 0 };

			_renderTargetSize.y = std::max(_renderTargetSize.y, (uint32_t)eyeSize.h);
			_renderTargetSize.x += eyeSize.w;
		});
		// Make the on screen window 1/4 the resolution of the render target
		_mirrorSize = _renderTargetSize;
		_mirrorSize /= 2; // was 4 before

	}

protected:
	GLFWwindow * createRenderingTarget(uvec2 & outSize, ivec2 & outPosition) override {
		return glfw::createWindow(_mirrorSize);
	}

	void initGl() override {
		GlfwApp::initGl();

		// Disable the v-sync for buffer swap
		glfwSwapInterval(0);

		ovrTextureSwapChainDesc desc = {};
		desc.Type = ovrTexture_2D;
		desc.ArraySize = 1;
		desc.Width = _renderTargetSize.x;
		desc.Height = _renderTargetSize.y;
		desc.MipLevels = 1;
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.SampleCount = 1;
		desc.StaticImage = ovrFalse;
		ovrResult result = ovr_CreateTextureSwapChainGL(_session, &desc, &_eyeTexture);
		_sceneLayer.ColorTexture[0] = _eyeTexture;
		if (!OVR_SUCCESS(result)) {
			FAIL("Failed to create swap textures");
		}

		int length = 0;
		result = ovr_GetTextureSwapChainLength(_session, _eyeTexture, &length);
		if (!OVR_SUCCESS(result) || !length) {
			FAIL("Unable to count swap chain textures");
		}
		for (int i = 0; i < length; ++i) {
			GLuint chainTexId;
			ovr_GetTextureSwapChainBufferGL(_session, _eyeTexture, i, &chainTexId);
			glBindTexture(GL_TEXTURE_2D, chainTexId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		// Set up the framebuffer object
		glGenFramebuffers(1, &_fbo);
		glGenRenderbuffers(1, &_depthBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
		glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, _renderTargetSize.x, _renderTargetSize.y);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		ovrMirrorTextureDesc mirrorDesc;
		memset(&mirrorDesc, 0, sizeof(mirrorDesc));
		mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		mirrorDesc.Width = _mirrorSize.x;
		mirrorDesc.Height = _mirrorSize.y;
		if (!OVR_SUCCESS(ovr_CreateMirrorTextureGL(_session, &mirrorDesc, &_mirrorTexture))) {
			FAIL("Could not create mirror texture");
		}
		glGenFramebuffers(1, &_mirrorFbo);

	}

	void update() final override
	{
		ovrInputState inputState;
		if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Touch, &inputState)))
		{
			// reset booleans
			cube_size_up = false;
			cube_size_down = false;
			cube_size_reset = false;

			// Logic to vary interocular distance
			if (inputState.Thumbstick[ovrHand_Right].x > 0) {
				//cout << "iod up" << endl;
				iod_up = true;
			}
			else if (inputState.Thumbstick[ovrHand_Right].x < 0) {
				//cout << "iod down" << endl;
				iod_down = true;
			}
			else if (inputState.Buttons & ovrButton_RThumb) {
				//cout << "iod reset" << endl;
				iod_reset = true;
			}

			////////////////////////
			// Logic to resize cubes
			if (inputState.Thumbstick[ovrHand_Left].x < 0) {
				//cout << "left thumbstick to the left" << endl;
				cube_size_down = true;
				//cube_size_up = false;
			}
			else if (inputState.Thumbstick[ovrHand_Left].x > 0) { 
				//cout << "left thumbstick to the right" << endl; 
				cube_size_up = true;
				//cube_size_down = false;
			}
			else if (inputState.Buttons && ovrButton_LThumb) {
				//cout << "left thubstick pressed in" << endl;
				cube_size_reset = true;
			}


			if (!inputState.Buttons) {
				isPressed = false;
			}

			///////////////////////////////
			// Logic to cycle between five modes with the 'A' button
			if ((inputState.Buttons & ovrButton_A) && !isPressed) { 
				//cout << "Button A pressed" << endl;
				isPressed = true;

				if (a1) {
					a1 = false;
					a2 = true;
					//cout << "monoscopic mode (left eye image rendered on both eyes)" << endl;
				}
				else if (a2) {
					a2 = false;
					a3 = true;
					//cout << "only rendering to left eye" << endl;
				}
				else if (a3) {
					a3 = false;
					a4 = true;
					//cout << "only rendering to right eye" << endl;
				}
				else if (a4) {
					a4 = false;
					a5 = true;
					//cout << "inverted stereo mode" << endl;
				}
				else if (a5) {
					a1 = true;
					a5 = false;
					//cout << "back to default mode" << endl;
				}
			}

			///////////////////////////////
			// Logic to cycle between five modes with the 'X' button
			else if ((inputState.Buttons & ovrButton_X) && !isPressed) {

				isPressed = true;

				if (x1) {
					x1 = false;
					x2 = true;
					cout << "showing just the sky box in stereo" << endl;
				}
				else if (x2) {
					x2 = false;
					x3 = true;
					cout << "showing just the sky box in mono" << endl;
				}
				else if (x3) {
					x3 = false;
					x4 = true;
					cout << "showing my room" << endl;
				}
				else if (x4) {
					x4 = false;
					x1 = true;
					cout << "showing the entire scene" << endl;
				}							
			}

			///////////////////////////////
			// Logic to cycle between four head tracking modes with the 'B' button
			else if ((inputState.Buttons & ovrButton_B) && !isPressed) {

				isPressed = true;

				if (b1) {
					b1 = false;
					b2 = true;
					cout << "orientation only (position frozen to what it just was before the mode was selected)" << endl;
				}
				else if (b2) {
					b2 = false;
					b3 = true;
					cout << "position only (orientation frozen to what it just was)" << endl;
				}
				else if (b3) {
					b3 = false;
					b4 = true;
					cout << "no tracking (position and orientation frozen to what they just were when the user pressed the button)" << endl;
				}
				else if (b4) {
					b4 = false;
					b1 = true;
					cout << "regular tracking (both position and orientation)" << endl;
				}

			}
			else if ((inputState.Buttons & ovrButton_Y) && !isPressed) {
				isPressed = true;
				if (superRotation) superRotation = false;
				else superRotation = true;
			}

			/////////////
			// change iod
			if (iod_up && iod < 0.3) {
				iod += 0.01;
			}
			else if (iod_down && iod > -0.1) {
				iod -= 0.01;
			}
			else if (iod_reset) {
				iod = original_iod;
			}
			_viewScaleDesc.HmdToEyePose[0].Position.x = (float)(-iod / 2);
			_viewScaleDesc.HmdToEyePose[1].Position.x = (float)(iod / 2);
		}

		// reset booleans
		iod_up = false;
		iod_down = false;
		iod_reset = false;
	}

	void onKey(int key, int scancode, int action, int mods) override {
		if (GLFW_PRESS == action) switch (key) {
		case GLFW_KEY_R:
			ovr_RecenterTrackingOrigin(_session);
			return;
		}

		GlfwApp::onKey(key, scancode, action, mods);
	}

	void draw() final override {

		// Query Touch controllers. Query their parameters:
		double displayMidpointSeconds = ovr_GetPredictedDisplayTime(_session, 0);
		ovrTrackingState trackState = ovr_GetTrackingState(_session, displayMidpointSeconds, ovrTrue);

		// Process controller status. Useful to know if controller is being used at all, and if the cameras can see it. 
		// Bits reported:
		// Bit 1: ovrStatus_OrientationTracked  = Orientation is currently tracked (connected and in use)
		// Bit 2: ovrStatus_PositionTracked     = Position is currently tracked (false if out of range)
		unsigned int handStatus[2];
		handStatus[0] = trackState.HandStatusFlags[0];
		handStatus[1] = trackState.HandStatusFlags[1];
		// Display status for debug purposes:
		//cerr << "handStatus[left]  = " << handStatus[ovrHand_Left] << endl;
		//cerr << "handStatus[right] = " << handStatus[ovrHand_Right] << endl;

		// Process controller position and orientation:
		ovrPosef handPoses[2];  // These are position and orientation in meters in room coordinates, relative to tracking origin. Right-handed cartesian coordinates.
								// ovrQuatf     Orientation;
								// ovrVector3f  Position;
		handPoses[0] = trackState.HandPoses[0].ThePose;
		handPoses[1] = trackState.HandPoses[1].ThePose;
		ovrVector3f handPosition[2];
		handPosition[0] = handPoses[0].Position;
		handPosition[1] = handPoses[1].Position;
		// Display positions for debug purposes:
		//cerr << "left hand position  = " << handPosition[ovrHand_Left].x << ", " << handPosition[ovrHand_Left].y << ", " << handPosition[ovrHand_Left].z << endl;
		//cerr << "right hand position = " << handPosition[ovrHand_Right].x << ", " << handPosition[ovrHand_Right].y << ", " << handPosition[ovrHand_Right].z << endl;
		/////////////////////////////////////////

		ovrPosef eyePoses[2];
		ovr_GetEyePoses(_session, frame, true, _viewScaleDesc.HmdToEyePose, eyePoses, &_sceneLayer.SensorSampleTime);

		///////////////////////////////
		///// head positions this frame
		headPos_left_curr = ovr::toGlm(eyePoses[ovrEye_Left]);
		headPos_right_curr = ovr::toGlm(eyePoses[ovrEye_Right]);
		
		/////////
		// do nothing on regular tracking mode (b1)

		if (b2) {
			// orientation only
			// position frozen to last frame
			headPos_left_curr[3] = headPos_left_prev[3];
			headPos_right_curr[3] = headPos_right_prev[3];
		}
		else if (b3) {
			// position only
			// orientation frozen to last frame
			headPos_left_curr[0] = headPos_left_prev[0];
			headPos_left_curr[1] = headPos_left_prev[1];
			headPos_left_curr[2] = headPos_left_prev[2];

			headPos_right_curr[0] = headPos_right_prev[0];
			headPos_right_curr[1] = headPos_right_prev[1];
			headPos_right_curr[2] = headPos_right_prev[2];
		}
		else if (b4) {
			// no tracking
			// position and orientation frozen to last frame
			headPos_left_curr = headPos_left_prev;
			headPos_right_curr = headPos_right_prev;
		}

		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(_session, _eyeTexture, &curIndex);
		GLuint curTexId;
		ovr_GetTextureSwapChainBufferGL(_session, _eyeTexture, curIndex, &curTexId);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/////// LOOK OVER HERE
		ovr::for_each_eye([&](ovrEyeType eye) {
			const auto& vp = _sceneLayer.Viewport[eye];
			glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
			_sceneLayer.RenderPose[eye] = eyePoses[eye];
			
			if (a1) {
				//renderScene(_eyeProjections[eye], ovr::toGlm(eyePoses[eye]));  // cse190: this is for normal stereo rendering

				// call renderScene() twice one time for each eye
				if (eye == ovrEye_Left) {
					//renderScene(_eyeProjections[ovrEye_Left], ovr::toGlm(eyePoses[ovrEye_Left]), true);
					renderScene(_eyeProjections[ovrEye_Left], headPos_left_curr, true);

				}
				else {
					//renderScene(_eyeProjections[ovrEye_Right], ovr::toGlm(eyePoses[ovrEye_Right]), false);
					renderScene(_eyeProjections[ovrEye_Right], headPos_right_curr, false);

				}			
			}

			else if (a2) {
				// render one eye's view to both eyes = monoscopic view
				/*renderScene(_eyeProjections[eye], ovr::toGlm(eyePoses[ovrEye_Left]), true);*/
				renderScene(_eyeProjections[eye], headPos_left_curr, true);
			}
			else if (a3) {
				// render to only left eye
				if (eye == ovrEye_Left) {
					//renderScene(_eyeProjections[ovrEye_Left], ovr::toGlm(eyePoses[ovrEye_Left]), true);
					renderScene(_eyeProjections[ovrEye_Left], headPos_left_curr, true);
				}
				
			}
			else if (a4) {
				// render to only right eye
				if (eye == ovrEye_Right) {
					//renderScene(_eyeProjections[ovrEye_Right], ovr::toGlm(eyePoses[ovrEye_Right]), false);

					renderScene(_eyeProjections[ovrEye_Right], headPos_right_curr, false);
				}
			}
			else if (a5) {
				// render left eye to right eye and vice versa - inverted stereo
				/*if (eye == ovrEye_Left) renderScene(_eyeProjections[ovrEye_Right], ovr::toGlm(eyePoses[ovrEye_Right]), false);
				if (eye == ovrEye_Right) renderScene(_eyeProjections[ovrEye_Left], ovr::toGlm(eyePoses[ovrEye_Left]), true);*/

				if (eye == ovrEye_Left) renderScene(_eyeProjections[ovrEye_Right], headPos_right_curr, false);
				if (eye == ovrEye_Right) renderScene(_eyeProjections[ovrEye_Left], headPos_left_curr, true);
			}

		});
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		ovr_CommitTextureSwapChain(_session, _eyeTexture);
		ovrLayerHeader* headerList = &_sceneLayer.Header;
		ovr_SubmitFrame(_session, frame, &_viewScaleDesc, &headerList, 1);

		GLuint mirrorTextureId;
		ovr_GetMirrorTextureBufferGL(_session, _mirrorTexture, &mirrorTextureId);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, _mirrorFbo);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTextureId, 0);
		glBlitFramebuffer(0, 0, _mirrorSize.x, _mirrorSize.y, 0, _mirrorSize.y, _mirrorSize.x, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);


		/////////// store head positions from the previous frame
		headPos_left_prev = headPos_left_curr;
		headPos_right_prev = headPos_right_curr;
	}

	/*virtual void renderScene(const glm::mat4 & projection, const glm::mat4 & headPose) = 0;*/
	virtual void renderScene(const glm::mat4 & projection, const glm::mat4 & headPose, bool isLeft) = 0;
};

//////////////////////////////////////////////////////////////////////
//
// The remainder of this code is specific to the scene we want to 
// render.  I use oglplus to render an array of cubes, but your 
// application would perform whatever rendering you want
//

// a class for encapsulating building and rendering an RGB cube
struct ColorCubeScene {

public:
	Cube * cube_1;
	Cube * skybox_left;
	Cube * skybox_right;
	Cube * skybox_room;

	GLuint cube_shader;

	vector<string> cube_faces = {
		"cube_pattern.ppm",
		"cube_pattern.ppm",
		"cube_pattern.ppm",
		"cube_pattern.ppm",
		"cube_pattern.ppm",
		"cube_pattern.ppm"
	};

	vector<string> skybox_faces_room = {
		// 2 are flipped horizontally
		// 3 are flipped vertically
		"skybox_room/px_2.ppm",
		"skybox_room/nx_2.ppm",
		"skybox_room/py_3.ppm",
		"skybox_room/ny_3.ppm",
		"skybox_room/nz_2.ppm",
		"skybox_room/pz_2.ppm",
	};

	vector<string> skybox_faces_left = {
		"skybox_leftEye/nx.ppm",
		"skybox_leftEye/px.ppm",
		"skybox_leftEye/py_2.ppm", // rotated 
		"skybox_leftEye/ny_2.ppm", // rotated
		"skybox_leftEye/nz.ppm",
		"skybox_leftEye/pz.ppm",
	};

	vector<string> skybox_faces_right = {
		"skybox_rightEye/nx.ppm",
		"skybox_rightEye/px.ppm",
		"skybox_rightEye/py_2.ppm", // rotated 
		"skybox_rightEye/ny_2.ppm", // rotated
		"skybox_rightEye/nz.ppm",
		"skybox_rightEye/pz.ppm",
	};

	const char * CUBE_VERT_PATH = "shader_cube.vert";
	const char * CUBE_FRAG_PATH = "shader_cube.frag";

	glm::mat4 cubeScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f)); // only mat used to scale cube

	ColorCubeScene() {
		skybox_left = new Cube(1, skybox_faces_left, true, true, false);

		skybox_right = new Cube(1, skybox_faces_right, true, false, false);

		skybox_room = new Cube(1, skybox_faces_room, true, false, true);

		cube_1 = new Cube(1, cube_faces, false, false, false); // first cube of size 1

		cube_shader = LoadShaders(CUBE_VERT_PATH, CUBE_FRAG_PATH);
	}

	~ColorCubeScene(){
		delete(skybox_left);
		delete(skybox_right);
		delete(cube_1);
		glDeleteProgram(cube_shader);
		// delete char * ?
	}

	void resetCubes() {
		cubeScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
	} 

	void scaleCubes(float val) {
		cubeScaleMat = cubeScaleMat * glm::scale(glm::mat4(1.0f), glm::vec3(val));
	}

	void render(const mat4 & projection, const mat4 & modelview, bool isLeftEye) {

		// change cubeScaleMat according to booleans
		if (cube_size_up) {
			if (cubeScaleMat[0][0] < 0.5f && cubeScaleMat[1][1] < 0.5f && cubeScaleMat[2][2] < 0.5f) {
				scaleCubes(1.01f);
			}
		}
		if (cube_size_down) {
			if (cubeScaleMat[0][0] > 0.01f && cubeScaleMat[1][1] > 0.01f && cubeScaleMat[2][2] > 0.01f) {
				scaleCubes(0.99f);
			}
		}

		if (cube_size_reset) {
			resetCubes();
		}

		glUseProgram(cube_shader);
		GLuint uProjection = glGetUniformLocation(cube_shader, "model");

		// render skybox
		glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 100.0f, 100.0f));
		glUniformMatrix4fv(uProjection, 1, GL_FALSE, &scaleMat[0][0]);

		// render in different modes 
		if (x1 || x2) {
			// render different texture images for left and right eye to create stereo effect
			if (isLeftEye) {
				//cout << "isLeftEye" << endl;
				skybox_left->draw(cube_shader, projection, modelview);
			}
			else {
				//cout << "isRightEye" << endl;
				skybox_right->draw(cube_shader, projection, modelview);
			}

			if (x1) {
				// render cubes
				// specify positions
				vec3 pos_1 = vec3(0.0f, 0.0f, -4.0f);
				vec3 pos_2 = vec3(0.0f, 0.0f, -8.0f);

				glm::mat4 posMat = glm::translate(glm::mat4(1.0f), pos_1);
				glm::mat4 posMat_in = glm::translate(glm::mat4(1.0f), -pos_1);
				glm::mat4 M = posMat * cubeScaleMat * posMat_in;

				// draw closer cube
				glUniformMatrix4fv(uProjection, 1, GL_FALSE, &M[0][0]);
				cube_1->draw(cube_shader, projection, modelview);

				posMat = glm::translate(glm::mat4(1.0f), pos_2);
				posMat_in = glm::translate(glm::mat4(1.0f), -pos_2);
				M = posMat * cubeScaleMat * posMat_in;

				// draw further cube
				glUniformMatrix4fv(uProjection, 1, GL_FALSE, &M[0][0]);
				cube_1->draw(cube_shader, projection, modelview);
			}
		}
		else if (x3) {
			// render just skybox in mono
			skybox_left->draw(cube_shader, projection, modelview);
		}
		else if (x4) {
			// render custom skybox
			skybox_room->draw(cube_shader, projection, modelview);
		}
	}
};


// An example application that renders a simple cube
class ExampleApp : public RiftApp {
	std::shared_ptr<ColorCubeScene> cubeScene;

public:
	ExampleApp() { }


protected:
	void initGl() override {
		RiftApp::initGl();
		glClearColor(0.2f, 0.3f, 0.8f, 1.0f); // change background color to light blue

		glEnable(GL_DEPTH_TEST);
		ovr_RecenterTrackingOrigin(_session);
		cubeScene = std::shared_ptr<ColorCubeScene>(new ColorCubeScene());
	}

	void shutdownGl() override {
		cubeScene.reset();
	}

	//void renderScene(const glm::mat4 & projection, const glm::mat4 & headPose) override {
	//	//cubeScene->render(projection, glm::inverse(headPose));
	//}

	mat3 computeRotation(float theta_x, float theta_y, float theta_z) {
		mat3 X, Y, Z = mat3(1.0f);

		X[1][1] = cosf(theta_x);
		X[2][1] = -sinf(theta_x);
		X[1][2] = sinf(theta_x);
		X[2][2] = cosf(theta_x);

		Y[0][0] = cosf(theta_y);
		Y[0][2] = -sinf(theta_y);
		Y[2][0] = sinf(theta_y);
		Y[2][2] = cosf(theta_y);

		Z[0][0] = cosf(theta_z);
		Z[0][1] = sinf(theta_z);
		Z[1][0] = -sinf(theta_z);
		Z[1][1] = cosf(theta_z);

		mat3 R = Z * Y * X;
		/*mat3 R = Z * X * Y;*/

		/*mat3 R = Y * X * Z;*/
		/*mat3 R = Y * Z * X;*/

		/*mat3 R = X * Z * Y;*/
		//mat3 R = X * Y * Z;

		return R;
	}

	// newly defined function
	// To freeze head rotation and/or position, manipulate mat4 headPose (see notes)
	void renderScene(const glm::mat4 & projection, const glm::mat4 & headPose, bool isLeft) {
		headPos_curr = headPose;

		if (!superRotation) {
			cubeScene->render(projection, glm::inverse(headPose), isLeft);
			if (!isPressed) {
				cout << "old head pose" << endl;				
				cout << "inverse(headPose)[0]: " << glm::inverse(headPose)[0].x << ", " << glm::inverse(headPose)[0].y << ", " << glm::inverse(headPose)[0].z << endl;
				cout << "inverse(headPose)[1]: " << glm::inverse(headPose)[1].x << ", " << glm::inverse(headPose)[1].y << ", " << glm::inverse(headPose)[1].z << endl;
				cout << "inverse(headPose)[2]: " << glm::inverse(headPose)[2].x << ", " << glm::inverse(headPose)[2].y << ", " << glm::inverse(headPose)[2].z << endl;
			}
		}
		else {
			// Super-rotation

			//// 1. compute euler angles from HMD's rotation matrix
			mat3 R = mat3(headPos_curr[0], headPos_curr[1], headPos_curr[2]);

			// euler angles for left eye
			/*theta_x_curr = atan2f(R[1][2], R[2][2]);
			theta_y_curr = atan2f(-R[0][2], sqrt(pow(R[1][2], 2) + pow(R[2][2], 2)));
			theta_z_curr = atan2f(R[0][1], R[0][0]);*/
			//////////////////////////////////////////

			theta_y_curr = atan2f(R[1][1], R[2][1]);
			theta_z_curr = atan2f(-R[0][1], sqrt(pow(R[1][1], 2) + pow(R[2][1], 2)));
			theta_x_curr = atan2f(R[0][0], R[0][2]);

			/*theta_y_curr = atan2f(R[1][0], R[2][0]);
			theta_z_curr = atan2f(-R[0][0], sqrt(pow(R[1][0], 2) + pow(R[2][0], 2)));
			theta_x_curr = atan2f(R[0][2], R[0][1]);*/

			/*theta_y_curr = atan2f(R[1][2], R[2][2]);
			theta_z_curr = atan2f(-R[0][2], sqrt(pow(R[1][2], 2) + pow(R[2][2], 2)));
			theta_x_curr = atan2f(R[0][1], R[0][0]);*/


			//cout << "theta_curr: " << theta_x_curr << ", " << theta_y_curr/* *2*/ << ", " << theta_z_curr << endl;

			//// 2. get the delta value of euler angles between two frames
			//// 3. double the delta values;
			//float delta_x = (theta_x_curr - theta_x_prev)/* * 2*/;
			//float delta_y = (theta_y_curr - theta_y_prev)/* * 2*/;
			//float delta_z = (theta_z_curr - theta_z_prev)/* * 2*/;

			//// 4. use the new delta values to compose a new rotation matrix
			// call function plug in delta values
			/*float x = theta_x_curr;
			float y = theta_y_curr * 2.0;
			float z = theta_z_curr;*/

			//mat3 new_R = computeRotation(theta_x_curr, y, theta_z_curr);

			mat3 new_R = computeRotation(theta_z_curr, theta_x_curr * 2.0, theta_y_curr);

			//// 5. assign new rotation matrix back to transformation matrix
			mat4 new_T = mat4(new_R);
			new_T[3] = headPos_curr[3];
			// do I need to put [0][3], [1][3], and [2][3] ?

			// 6. done
			headPos_curr = new_T;
			cubeScene->render(projection, glm::inverse(headPos_curr), isLeft);

			if (isPressed) {
				cout << "new head pose" << endl;
				cout << "inverse(headPose)[0]: " << glm::inverse(headPose)[0].x << ", " << glm::inverse(headPose)[0].y << ", " << glm::inverse(headPose)[0].z << endl;
				cout << "inverse(headPose)[1]: " << glm::inverse(headPose)[1].x << ", " << glm::inverse(headPose)[1].y << ", " << glm::inverse(headPose)[1].z << endl;
				cout << "inverse(headPose)[2]: " << glm::inverse(headPose)[2].x << ", " << glm::inverse(headPose)[2].y << ", " << glm::inverse(headPose)[2].z << endl;
			}
		}

		headPos_prev = headPos_curr;
	}
};
 
// Execute our example class
int main(int argc, char** argv)
{
	int result = -1;
	try {
		if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
			FAIL("Failed to initialize the Oculus SDK");
		}
		result = ExampleApp().run();
	}
	catch (std::exception & error) {
		OutputDebugStringA(error.what());
		std::cerr << error.what() << std::endl;
	}
	ovr_Shutdown();
	return result;
}