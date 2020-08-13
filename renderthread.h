#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
// #include <glm/gtc/swizzle.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <assimp/postprocess.h>
#include <chrono>
#include "Model.h"
#include "Shader.h"
#include <thread>

#include "gpu_vector.h"
#include "Input.h"
#include <atomic>
#include "game_object.h"
#include "rendering.h"
#include "game_engine_components.h"
#include "physics.h"
// #include "particles.h"
#include "physics_.h"
#include "audio.h"
#include "gui.h"
// #include "renderTexture.h"
// #include "lighting.h"

using namespace std;

GLFWwindow *window;
GLdouble lastFrame = 0;
Shader *shadowShader;
Shader *OmniShadowShader;

bool hideMouse = true;
atomic<bool> renderDone(false);
atomic<bool> renderThreadReady(false);
bool recieveMouse = true;


/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////GL WINDOW FUNCTIONS////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void window_close_callback(GLFWwindow *window)
{
	//if (!time_to_close)
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}
void mouseFrameBegin()
{
	Input.Mouse.xOffset = Input.Mouse.yOffset = Input.Mouse.mouseScroll = 0;
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button >= 0 && button < 1024)
	{
		if (action == GLFW_PRESS)
		{
			Input.Mouse.mouseButtons[button] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			Input.Mouse.mouseButtons[button] = false;
		}
	}
}

void MouseCallback(GLFWwindow *window, double xPos, double yPos)
{
	if (recieveMouse)
	{
		if (Input.Mouse.firstMouse)
		{
			Input.Mouse.lastX = xPos;
			Input.Mouse.lastY = yPos;
			Input.Mouse.firstMouse = false;
		}

		Input.Mouse.xOffset = xPos - Input.Mouse.lastX;
		Input.Mouse.yOffset = Input.Mouse.lastY - yPos;

		Input.Mouse.lastX = xPos;
		Input.Mouse.lastY = yPos;
	}
	//camera.ProcessMouseMovement(xOffset, yOffset);
}
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			Input.keys[key] = true;
			Input.keyDowns[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			Input.keys[key] = false;
		}
	}
}
void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
	Input.Mouse.mouseScroll = yOffset;
	//camera.ProcessMouseScroll(yOffset);
}
void window_size_callback(GLFWwindow *window, int width, int height)
{
	glfwSetWindowSize(window, width, height);
}
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	SCREEN_WIDTH = width;
	SCREEN_HEIGHT = height;
	glViewport(0, 0, width, height);
}
bool eventsPollDone;
void updateTiming()
{
	Input.resetKeyDowns();
	mouseFrameBegin();
	glfwPollEvents();

	double currentFrame = glfwGetTime();
	Time.unscaledTime = currentFrame;
	Time.unscaledDeltaTime = currentFrame - lastFrame;
	Time.deltaTime = _min(Time.unscaledDeltaTime, 1.f / 10.f) * Time.timeScale;
	Time.time += Time.deltaTime;
	Time.timeBuffer.add(Time.unscaledDeltaTime);
	lastFrame = currentFrame;
	Time.unscaledSmoothDeltaTime = Time.timeBuffer.getAverageValue();
	eventsPollDone = true;
}
struct renderData{
	glm::mat4 vp; glm::mat4 view; glm::vec3 camPos; glm::vec2 screen; glm::vec3 cullPos; glm::mat3 camInv; glm::mat4 rot; glm::mat4 proj;
};
int frameCounter = 0;

void renderThreadFunc()
{

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "game engine", nullptr, nullptr);
	if (window == nullptr)
	{
		cout << "failed to create GLFW window" << endl;
		glfwTerminate();

		throw EXIT_FAILURE;
	}

/////////////////////////////////////////////////////////////
///////////////////////// GUI ////////////////////////////////
/////////////////////////////////////////////////////////////

	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls



    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    // ImGui::StyleColorsLight();

	auto font_default = io.Fonts->AddFontDefault();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	if (hideMouse)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetWindowCloseCallback(window, window_close_callback);

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	//glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		cout << "failed to initialize GLEW" << endl;

		throw EXIT_FAILURE;
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// glEnable(GLEW_ARB_compute_shader);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_COMPONENT32);
	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_CULL_FACE);
	

	Shader matProgram("res/shaders/mat.comp");
	// Shader shaderLightingPass("res/shaders/defferedLighting.vert","res/shaders/defferedLighting.frag");
	// Shader quadShader("res/shaders/defLighting.vert","res/shaders/defLighting.frag");

	// shaderLightingPass.use();
    // shaderLightingPass.setInt("gAlbedoSpec", 0);
    // shaderLightingPass.setInt("gPosition", 1);
    // shaderLightingPass.setInt("gNormal", 2);
	// shaderLightingPass.setInt("gDepth", 3);


	// quadShader.use();
    // quadShader.setInt("gAlbedoSpec", 0);
    // quadShader.setInt("gPosition", 1);
    // quadShader.setInt("gNormal", 2);

	GLint max_buffers;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &max_buffers);
	cout << max_buffers << endl;
	GLint maxAtt = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAtt);
	cout << maxAtt << endl;

	// shadowShader = new Shader("res/shaders/directional_shadow_map.vert", "res/shaders/directional_shadow_map.frag", false);
	// OmniShadowShader = new Shader("res/shaders/omni_shadow_map.vert", "res/shaders/omni_shadow_map.geom", "res/shaders/omni_shadow_map.frag", false);
	GPU_MATRIXES = new gpu_vector_proxy<matrix>();
	__RENDERERS = new gpu_vector<__renderer>();
	__RENDERERS->ownStorage();
	__renderer_offsets = new gpu_vector<GLuint>();
	__renderer_offsets->ownStorage();
	_renderer_radii = new gpu_vector<GLfloat>();
	_renderer_radii->ownStorage();
	initTransform();
	initParticles();
	particle_renderer::init();
	lighting::init();
	timer stopWatch;

	renderTexture colors;
	colors.scr_width = SCREEN_WIDTH;
	colors.scr_height = SCREEN_HEIGHT;
	colors.init();

	renderDone.store(true);
	renderThreadReady.exchange(true);

	while (true)
	{
		// log("render loop");
		// log(to_string(renderWork.size()));
		if (renderWork.size() > 0)
		{
			renderLock.lock();
			renderJob* rj = renderWork.front();
			renderWork.pop();
			switch (rj->type)
			{
			case renderNum::doFunc: //    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				renderLock.unlock();

				rj->work();

				if(rj->completed == 0){
					rj->completed = 1;
					while(rj->completed != 2){
						this_thread::sleep_for(1ns);
					}
				}
				
				break;
			case renderNum::render:
			{
				gpuTimer gt_;
				timer cpuTimer;
				stopWatch.start();
				updateTiming();


				auto cameras = COMPONENT_LIST(_camera);

				cpuTimer.start();
				gt_.start();
				GPU_TRANSFORMS->tryRealloc(TRANSFORMS.size());
				GPU_TRANSFORMS_UPDATES->tryRealloc(TRANSFORMS.size());
				transformIds->tryRealloc(TRANSFORMS.size());

				// GLuint offset = 0;
				// transformIdsToBuffer.clear();
				// transformsToBuffer.clear();
				// for(int i = 0; i < transformIdThreadcache.size(); ++i){
				// 	transformIdsToBuffer.insert(transformIdsToBuffer.end(),transformIdThreadcache[i].begin(),transformIdThreadcache[i].end());
				// 	transformsToBuffer.insert(transformsToBuffer.end(),transformThreadcache[i].begin(),transformThreadcache[i].end());
				// 	// transformIds->bufferData(transformIdThreadcache[i],offset,transformIdThreadcache[i].size());
				// 	// GPU_TRANSFORMS_UPDATES->bufferData(transformThreadcache[i],offset,transformThreadcache[i].size());
				// 	// offset += transformIdThreadcache[i].size();
				// }
				transformIds->bufferData(transformIdsToBuffer);
				GPU_TRANSFORMS_UPDATES->bufferData(transformsToBuffer);
				matProgram.use();
				GPU_TRANSFORMS->bindData(0);
				transformIds->bindData(6);
				GPU_TRANSFORMS_UPDATES->bindData(7);

				matProgram.setInt("stage",-1);
				matProgram.setUint("num",transformsToBuffer.size());
				glDispatchCompute(transformsToBuffer.size() / 64 + 1, 1, 1);
				glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
				appendStat("transforms buffer", gt_.stop());
				appendStat("transforms buffer cpu", cpuTimer.stop());
				
				cpuTimer.start();
				uint emitterInitCount = emitterInits.size();
				prepParticles();

				_camera::initPrepRender(matProgram);
				appendStat("render init cpu", cpuTimer.stop());


				cpuTimer.start();
				updateParticles(mainCamPos,emitterInitCount);
				appendStat("particles compute", cpuTimer.stop());
				vector<renderData> r_d;
				for(_camera& c : cameras->data.data){
					cpuTimer.start();
					gt_.start();
					c.prepRender(matProgram);
					appendStat("matrix compute", gt_.stop());
					appendStat("matrix compute", cpuTimer.stop());
					r_d.push_back(renderData());
					r_d.back().vp = c.proj * c.rot * c.view;
					r_d.back().rot = c.rot;
					r_d.back().view = c.view;
					r_d.back().camPos = c.pos;
					r_d.back().screen = c.screen;
					r_d.back().cullPos = c.cullpos;
					r_d.back().camInv = c.camInv;
					r_d.back().proj = c.proj;


					// sort particles
					timer t;
					t.start();
					if(!c.lockFrustum)
						particle_renderer::setCamCull(c.camInv,c.cullpos);
					particle_renderer::sortParticles(c.proj * c.rot * c.view, c.rot * c.view, mainCamPos,c.screen);
					appendStat("particles sort", t.stop());
				}


				int k = 0;
				plm.gpu_pointLights->bufferData();

				for(_camera& c : cameras->data.data){
					c.render();
				}
				renderLock.unlock();


/////////////////////////////////////////////////////////////
///////////////////////// GUI ////////////////////////////////
/////////////////////////////////////////////////////////////
				cpuTimer.start();
				gt_.start();
				IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!"); // Exceptionally add an extra assert here for people confused with initial dear imgui setup
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

				ImGui::PushFont(font_default);

				for(auto& i : gui::gui_windows){
					i->render();
				}
				ImGui::PopFont();
				// Rendering
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
				appendStat("render gui", gt_.stop());
				appendStat("render gui", cpuTimer.stop());

/////////////////////////////////////////////////////////////
///////////////////////// GUI ////////////////////////////////
/////////////////////////////////////////////////////////////
				// colors.blit();

				glfwSwapBuffers(window);
				// glFlush();
				appendStat("render", stopWatch.stop());
				//renderDone.store(true);
			}
				break;
			case renderNum::rquit:
				particle_renderer::end();
				while (gpu_buffers.size() > 0)
				{
					(gpu_buffers.begin()->second)->deleteBuffer();
				}
				destroyRendering();


				// Cleanup gui
				ImGui_ImplOpenGL3_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
				
				glFlush();
				glfwTerminate();
				renderThreadReady.exchange(false);

				renderLock.unlock();
				delete rj;

				return;
			default:
				break;
			}
			delete rj;
		}
		// else
		// {
			this_thread::sleep_for(1ns);
		// }
	}
}
