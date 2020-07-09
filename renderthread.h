#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_SWIZZLE 
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
#include "particles.h"
#include "physics_.h"
#include "audio.h"
#include "gui.h"
#include "renderTexture.h"
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


vector<glm::vec4> lightPos;
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
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

	glEnable(GLEW_ARB_compute_shader);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH32F_STENCIL8);
	glEnable(GL_DEPTH_CLAMP);

	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Shader matProgram("res/shaders/mat.comp");
	Shader shaderLightingPass("res/shaders/defferedLighting.vert","res/shaders/defferedLighting.frag");
	Shader quadShader("res/shaders/defLighting.vert","res/shaders/defLighting.frag");

	shaderLightingPass.use();
    shaderLightingPass.setInt("gAlbedoSpec", 0);
    shaderLightingPass.setInt("gPosition", 1);
    shaderLightingPass.setInt("gNormal", 2);


	quadShader.use();
    quadShader.setInt("gAlbedoSpec", 0);
    quadShader.setInt("gPosition", 1);
    quadShader.setInt("gNormal", 2);

	GLint max_buffers;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &max_buffers);
	cout << max_buffers << endl;
	GLint maxAtt = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAtt);
	cout << maxAtt << endl;

	// shadowShader = new Shader("res/shaders/directional_shadow_map.vert", "res/shaders/directional_shadow_map.frag", false);
	// OmniShadowShader = new Shader("res/shaders/omni_shadow_map.vert", "res/shaders/omni_shadow_map.geom", "res/shaders/omni_shadow_map.frag", false);
	initTransform();
	initParticles();
	particle_renderer::init();

	timer stopWatch;


	// for(int i =0; i < 100; i++){
	// 	lightPos.push_back(randomSphere() * glm::sqrt(randf()) * 20.f);
	// 	lightPos.back().y = fmod(lightPos.back().y,3.f);
	// }
	for(int i = 0; i < 10; i++){
		for(int j = 0; j < 10; j++){
			lightPos.push_back(glm::vec4(i * 2 - 10,-0.5, j * 2 - 10,1));
		}
	}
	gpu_vector_proxy<glm::vec4>* gpu_lights = new gpu_vector_proxy<glm::vec4>();

	renderTexture gBuffer;
	gBuffer.scr_width = SCREEN_WIDTH;
	gBuffer.scr_height = SCREEN_HEIGHT;
	gBuffer.init();
	gBuffer.addColorAttachment("gAlbedoSpec",renderTextureType::UNSIGNED_BYTE,0);
	gBuffer.addColorAttachment("gPosition",renderTextureType::FLOAT,1);
	gBuffer.addColorAttachment("gNormal",renderTextureType::FLOAT,2);
	gBuffer.addDepthBuffer();
	gBuffer.finalize();


	renderTexture colors;
	colors.scr_width = SCREEN_WIDTH;
	colors.scr_height = SCREEN_HEIGHT;
	colors.init();

	Model lightVolumeModel("res/models/cube/cube.obj");
	lightVolumeModel.loadModel();
	lightVolume lv;
	lv.indices = lightVolumeModel.meshes[0].indices;
	lv.vertices = lightVolumeModel.meshes[0].vertices;
	lv.setupMesh();

	auto SetLightingUniforms = [&](float farPlane, glm::vec3 viewPos, glm::mat4 vp, int i){
		glm::mat4 mvp = vp * glm::translate(lightPos[i].xyz()) * glm::scale(glm::vec3(10));
		glUniformMatrix4fv(
			glGetUniformLocation(shaderLightingPass.Program, "mvp"),
			1,
			GL_FALSE,
			glm::value_ptr(mvp));
		glUniform1f(
			glGetUniformLocation(shaderLightingPass.Program, "FC"),
			2.0 / log2(farPlane + 1));
		glUniform3fv(
			glGetUniformLocation(shaderLightingPass.Program, "lightPos"),
			1,
			glm::value_ptr(lightPos[i]));
		vec3 col(1,1,1);
		glUniform3fv(
			glGetUniformLocation(shaderLightingPass.Program, "lightColor"),
			1,
			glm::value_ptr(col));
		glUniform3fv(
			glGetUniformLocation(shaderLightingPass.Program, "viewPos"),
			1,
			glm::value_ptr(viewPos));
	};

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
			case doFunc: //    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
				gpuTimer gt;

				stopWatch.start();
				updateTiming();


				auto cameras = ((componentStorage<_camera>*)allcomponents.at(typeid(_camera).hash_code()));

				GPU_TRANSFORMS->tryRealloc(TRANSFORMS.size());
				GPU_TRANSFORMS_UPDATES->tryRealloc(TRANSFORMS.size());
				transformIds->tryRealloc(TRANSFORMS.size());
				gt.start();

				GLuint offset = 0;
				for(int i = 0; i < transformIdThreadcache.size(); ++i){
					transformIds->bufferData(transformIdThreadcache[i],offset,transformIdThreadcache[i].size());
					GPU_TRANSFORMS_UPDATES->bufferData(transformThreadcache[i],offset,transformThreadcache[i].size());
					offset += transformIdThreadcache[i].size();
				}

				matProgram.Use();
				GPU_TRANSFORMS->bindData(0);
				transformIds->bindData(6);
				GPU_TRANSFORMS_UPDATES->bindData(7);

				glUniform1i(glGetUniformLocation(matProgram.Program, "stage"), -1);
				glUniform1ui(glGetUniformLocation(matProgram.Program, "num"), offset);
				glDispatchCompute(offset / 64 + 1, 1, 1);
				glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
				appendStat("transforms buffer", gt.stop());

				uint emitterInitCount = emitterInits.size();
				prepParticles();
				glFlush();

				_camera::initPrepRender(matProgram);


				gt.start();
				updateParticles(mainCamPos,emitterInitCount); // change orient to camera in sort particles
				appendStat("particles compute", gt.stop());
				vector<renderData> r_d;
				for(_camera& c : cameras->data.data){
					gt.start();
					c.prepRender(matProgram);
					appendStat("matrix compute", gt.stop());
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
				renderLock.unlock();

				int k = 0;
				for(_camera& c : cameras->data.data){

					// 1. geometry pass: render all geometric/color data to g-buffer 
					glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
					gBuffer.resize(SCREEN_WIDTH,SCREEN_HEIGHT);
					gBuffer.use();
					// colors.use();
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
					gt.start();

					glEnable(GL_DEPTH_TEST);
					glDepthFunc(GL_LESS);    
					glDepthMask(GL_TRUE);
					glEnable(GL_CULL_FACE);
					glCullFace(GL_BACK);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					c.render();

					appendStat("render cam", gt.stop());

					// // 2. lighting pass
					// gBuffer.blit(0,SCREEN_WIDTH,SCREEN_HEIGHT);
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					

					gpu_lights->bindData(0);
					gpu_lights->bufferData(lightPos);
					quadShader.use();
					glUniform3fv(
						glGetUniformLocation(quadShader.Program, "viewPos"),
						1,
						glm::value_ptr(c.pos));
	
					quadShader.setInt("gAlbedoSpec", 0);
					quadShader.setInt("gPosition", 1);
					quadShader.setInt("gNormal", 2);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gAlbedoSpec"));
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gPosition"));
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gNormal"));
					renderQuad();
					

					// glEnable(GL_DEPTH_CLAMP);
					glDisable(GL_DEPTH_TEST);  
					// glDisable(GL_CULL_FACE);
					// glDepthFunc(GL_LEQUAL);  
					glCullFace(GL_FRONT);  
					glDepthMask(GL_FALSE);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					glBlendEquation(GL_FUNC_ADD);
					shaderLightingPass.use();
					shaderLightingPass.setInt("gAlbedoSpec", 0);
					shaderLightingPass.setInt("gPosition", 1);
					shaderLightingPass.setInt("gNormal", 2);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gAlbedoSpec"));
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gPosition"));
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, gBuffer.getTexture("gNormal"));
					
					glViewport(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
					for(int i = 0; i < lightPos.size(); i++){
						SetLightingUniforms(c.farPlane,c.pos,c.proj * c.rot * c.view, i);
						lv.Draw(1);
					}
					glDepthMask(GL_TRUE);
					// glDisable(GL_DEPTH_CLAMP);

					// // render particle
					// gt.start();
					// glDisable(GL_CULL_FACE);
					// glDepthMask(GL_FALSE);
					// particle_renderer::drawParticles(r_d[k].view, r_d[k].rot, r_d[k].proj);
					// appendStat("render particles", gt.stop());
					// glDepthMask(GL_TRUE);
				}

/////////////////////////////////////////////////////////////
///////////////////////// GUI ////////////////////////////////
/////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////
///////////////////////// GUI ////////////////////////////////
/////////////////////////////////////////////////////////////
				// colors.blit();

				glfwSwapBuffers(window);
				glFlush();
				appendStat("render", stopWatch.stop());
				//renderDone.store(true);
			}
				break;
			case rquit:
				particle_renderer::end();
				while (gpu_buffers.size() > 0)
				{
					(gpu_buffers.begin()->second)->deleteBuffer();
				}

			// Cleanup
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
