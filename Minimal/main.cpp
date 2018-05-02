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

bool iod_up = false; // true when RThumbStick to right
bool iod_down = false; // true when RThumbStick to left
bool iod_reset = false; // true when RThumbStick pressed in

// Button X controls
bool x1; // show entire scene (cubes and sky box in stereo)
bool x2; // show just the sky box in stereo
bool x3; // show just the sky box in mono

// Button A controls
bool a1 = true; // 3D stereo
bool a2 = false; // mono (the same image rendered on both eyes)
bool a3 = false; // left eye only (right eye black)
bool a4 = false; // right eye only (left eye black)
bool a5 = false; // inverted stereo (left eye image rendered to right eye and vice versa)


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

	ovrEyeRenderDesc _eyeRenderDescs[2];

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
			//////////////
			_viewScaleDesc.HmdToEyePose[eye] = erd.HmdToEyePose; // cse190: adjust the eye separation here - need to use 3D vector from central point on Rift for each eye
			
			

			/*if (iod_up) {
				erd.HmdToEyePose.Position.x = erd.HmdToEyePose.Position.x + 0.01f;
				_viewScaleDesc.HmdToEyePose[eye] = erd.HmdToEyePose;
			}
			else {
				_viewScaleDesc.HmdToEyePose[eye] = erd.HmdToEyePose;
			}*/

			ovrFovPort & fov = _sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
			auto eyeSize = ovr_GetFovTextureSize(_session, eye, fov, 1.0f);
			_sceneLayer.Viewport[eye].Size = eyeSize;
			_sceneLayer.Viewport[eye].Pos = { (int)_renderTargetSize.x, 0 };

			_renderTargetSize.y = std::max(_renderTargetSize.y, (uint32_t)eyeSize.h);
			_renderTargetSize.x += eyeSize.w;
		});
		// Make the on screen window 1/4 the resolution of the render target
		_mirrorSize = _renderTargetSize;
		_mirrorSize /= 4;

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
		cube_size_up = false;
		cube_size_down = false;
		cube_size_reset = false;

		iod_up = false;
		iod_down = false;
		iod_reset = false;

		ovrInputState inputState;
		if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Touch, &inputState)))
		{
			/*if (inputState.HandTrigger[ovrHand_Right] > 0.5f)   cerr << "right middle trigger pressed" << endl;
			if (inputState.IndexTrigger[ovrHand_Right] > 0.5f)	cerr << "right index trigger pressed"  << endl;
			if (inputState.HandTrigger[ovrHand_Left] > 0.5f)    cerr << "left middle trigger pressed"  << endl;
			if (inputState.IndexTrigger[ovrHand_Left] > 0.5f)	cerr << "left index trigger pressed"   << endl;
			if (inputState.Buttons>0) cerr << "Button state:" << inputState.Buttons << endl;*/
			// cse190: no need to print the above messages


			// Logic to vary interocular distance
			if (inputState.Thumbstick[ovrHand_Right].x > 0) {
				cout << "iod up" << endl;
				iod_up = true;
			}
			else if (inputState.Thumbstick[ovrHand_Right].x < 0) {
				cout << "iod down" << endl;
				iod_down = true;
			}
			else if (inputState.Buttons & ovrButton_RThumb) {
				cout << "iod reset" << endl;
				iod_reset = true;
			}


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

			// Logic to cycle between five modes with the 'A' button
			if (inputState.Buttons == 1 && ovrButton_A) {  // how to detect button press for only one frame?
				cout << "Button A pressed" << endl;

				if (a1) {
					a1 = false;
					a2 = true;
					/*a3 = false;
					a4 = false;
					a5 = false;*/
					cout << "monoscopic mode (left eye image rendered on both eyes)" << endl;
				}
				else if (a2) {
					//a1 = false;
					a2 = false;
					a3 = true;
					/*a4 = false;
					a5 = false;*/
					cout << "only rendering to left eye" << endl;
				}
				else if (a3) {
					/*a1 = false;
					a2 = false;*/
					a3 = false;
					a4 = true;
					//a5 = false;
					cout << "only rendering to right eye" << endl;
				}
				else if (a4) {
					/*a1 = false;
					a2 = false;
					a3 = false;*/
					a4 = false;
					a5 = true;
					cout << "inverted stereo mode" << endl;
				}
				else if (a5) {
					a1 = true;
					/*a2 = false;
					a3 = false;
					a4 = false;*/
					a5 = false;
					cout << "back to default mode" << endl;
				}
			}

		}
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

		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(_session, _eyeTexture, &curIndex);
		GLuint curTexId;
		ovr_GetTextureSwapChainBufferGL(_session, _eyeTexture, curIndex, &curTexId);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
		
		//glClearColor(0.2f, 0.3f, 0.8f, 1.0f); // change background color to light blue
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/////// LOOK OVER HERE
		ovr::for_each_eye([&](ovrEyeType eye) {
			const auto& vp = _sceneLayer.Viewport[eye];
			glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
			_sceneLayer.RenderPose[eye] = eyePoses[eye];
			
			if (a1) {
				//renderScene(_eyeProjections[eye], ovr::toGlm(eyePoses[eye]));  // cse190: this is for normal stereo rendering

				// TODO call this twice one time for each eye
				
				if (eye == ovrEye_Left) {
					renderScene(_eyeProjections[ovrEye_Left], ovr::toGlm(eyePoses[ovrEye_Left]), true);
				}
				else {
					renderScene(_eyeProjections[ovrEye_Right], ovr::toGlm(eyePoses[ovrEye_Right]), false);
				}			
			}

			//renderScene(_eyeProjections[eye], ovr::toGlm(eyePoses[ovrEye_Left]));  // cse190: use eyePoses[ovrEye_Left] to render one eye's view to both eyes = monoscopic view
//			if (eye==ovrEye_Left) renderScene(_eyeProjections[eye], ovr::toGlm(eyePoses[eye]));  // cse190: this is how to render to only one eye

			//else if (a2) {
			//	renderScene(_eyeProjections[eye], ovr::toGlm(eyePoses[ovrEye_Right]));  // cse190: use eyePoses[ovrEye_Left] to render one eye's view to both eyes = monoscopic view
			//}
			//else if (a3) {
			//	if (eye == ovrEye_Left) renderScene(_eyeProjections[ovrEye_Left], ovr::toGlm(eyePoses[ovrEye_Left]));  // cse190: this is how to render to only one eye
			//}
			//else if (a4) {
			//	if (eye == ovrEye_Right) renderScene(_eyeProjections[ovrEye_Right], ovr::toGlm(eyePoses[ovrEye_Right]));  // cse190: this is how to render to only one eye
			//}
			//else if (a5) {
			//	if (eye == ovrEye_Left) renderScene(_eyeProjections[ovrEye_Right], ovr::toGlm(eyePoses[ovrEye_Right]));
			//	if (eye == ovrEye_Right) renderScene(_eyeProjections[ovrEye_Left], ovr::toGlm(eyePoses[ovrEye_Left]));
			//}

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

	GLuint cube_shader;

	vector<string> cube_faces = {
		"cube_pattern.ppm",
		"cube_pattern.ppm",
		"cube_pattern.ppm",
		"cube_pattern.ppm",
		"cube_pattern.ppm",
		"cube_pattern.ppm"
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
		skybox_left = new Cube(1, skybox_faces_left, true, true);

		skybox_right = new Cube(1, skybox_faces_right, true, false);

		cube_1 = new Cube(1, cube_faces, false, false); // first cube of size 1

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
	} //

	glm::mat4 scaleCubes(float val) {
		//cubeScaleMat = glm::scale(cubeScaleMat, glm::vec3(val, val, val));
		glm::mat4 mat = cubeScaleMat * glm::scale(glm::mat4(1.0f), glm::vec3(val));
		return mat;
	}

	//void OBJObject::scale(float val) {
	//	scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(val));
	//	this->toWorld = this->toWorld * scaleMat;
	//	sVal *= val;
	//}

	void render(const mat4 & projection, const mat4 & modelview, bool isLeftEye) {

		glUseProgram(cube_shader);

		GLuint uProjection = glGetUniformLocation(cube_shader, "model");

		// render skybox
		glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 100.0f, 100.0f));
		glUniformMatrix4fv(uProjection, 1, GL_FALSE, &scaleMat[0][0]);

		// render different texture images for left and right eye to create stereo effect
		if (isLeftEye) {
			skybox_left->draw(cube_shader, projection, modelview);
		}
		else {
			skybox_right->draw(cube_shader, projection, modelview);
		}	

		// render cubes
		// scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.12f, 0.12f, 0.12f));

		vec3 pos_1 = vec3(0.0f, 0.0f, -4.0f);
		vec3 pos_2 = vec3(0.0f, 0.0f, -8.0f);

		glm::mat4 posMat = glm::translate(glm::mat4(1.0f), pos_1);
		glm::mat4 M = cubeScaleMat * posMat;
		//glm::mat4 M = cubeScaleMat * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)) * posMat; // works to scale up

		// draw closer cube
		glUniformMatrix4fv(uProjection, 1, GL_FALSE, &M[0][0]);
		cube_1->draw(cube_shader, projection, modelview);

		posMat = glm::translate(glm::mat4(1.0f), pos_2);
		M = cubeScaleMat * posMat;

		// draw further cube
		glUniformMatrix4fv(uProjection, 1, GL_FALSE, &M[0][0]);
		cube_1->draw(cube_shader, projection, modelview);

	}

	//void render(const mat4 & projection, const mat4 & modelview) { 

	//	// set scaling range

	//	// change cubeScaleMat according to booleans
	//	if (cube_size_up) {
	//		if (cubeScaleMat[3][0] < 0.5f && cubeScaleMat[3][1] < 0.5f && cubeScaleMat[3][2] < 0.5f) {
	//			cubeScaleMat = scaleCubes(1.01f);
	//		}
	//	}
	//	if (cube_size_down) {
	//		if (cubeScaleMat[3].x > 0.01f && cubeScaleMat[3].y > 0.01f && cubeScaleMat[3].z > 0.01f) {
	//			cubeScaleMat = scaleCubes(0.09f);
	//		}			
	//	}

	//	if (cube_size_reset) {
	//		resetCubes();
	//	}
	//		
	///*	glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(1.05f));
	//	cubeScaleMat = cubeScaleMat * S;*/
	//	//cubeScaleMat = cubeScaleMat * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));

	//	glUseProgram(cube_shader);

	//	GLuint uProjection = glGetUniformLocation(cube_shader, "model");

	//	// render skybox
	//	glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 100.0f, 100.0f));
	//	glUniformMatrix4fv(uProjection, 1, GL_FALSE, &scaleMat[0][0]);

	//	skybox_left->draw(cube_shader, projection, modelview);

	//	// render cubes
	//	// scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.12f, 0.12f, 0.12f));

	//	vec3 pos_1 = vec3(0.0f, 0.0f, -4.0f);
	//	vec3 pos_2 = vec3(0.0f, 0.0f, -8.0f);

	//	glm::mat4 posMat = glm::translate(glm::mat4(1.0f), pos_1);
	//	glm::mat4 M = cubeScaleMat * posMat;
	//	//glm::mat4 M = cubeScaleMat * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)) * posMat; // works to scale up

	//	// draw closer cube
	//	glUniformMatrix4fv(uProjection, 1, GL_FALSE, &M[0][0]);
	//	cube_1->draw(cube_shader, projection, modelview);

	//	posMat = glm::translate(glm::mat4(1.0f), pos_2);
	//	M = cubeScaleMat * posMat;

	//	// draw further cube
	//	glUniformMatrix4fv(uProjection, 1, GL_FALSE, &M[0][0]);
	//	cube_1->draw(cube_shader, projection, modelview);


	//}
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

	// newly defined function
	void renderScene(const glm::mat4 & projection, const glm::mat4 & headPose, bool isLeft) {
		cubeScene->render(projection, glm::inverse(headPose), isLeft);
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