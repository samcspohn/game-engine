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
#include "camera.h"
// #include "physics.h"
// #include "particles.h"
// #include "Components/physics.h"
#include "audio.h"
#include "gui.h"
#include "gpu_sort.h"
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
			ImGui::GetIO().KeysDown[key] = true;
			Input.keyDowns[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			Input.keys[key] = false;
			ImGui::GetIO().KeysDown[key] = false;
		}
	}
}
void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
	Input.Mouse.mouseScroll = yOffset;
	ImGuiIO &io = ImGui::GetIO();
	io.MouseWheelH += (float)xOffset;
	io.MouseWheel += (float)yOffset;
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
// bool eventsPollDone;
void updateTiming()
{
	Input.resetKeyDowns();
	mouseFrameBegin();
	glfwPollEvents();

	double currentFrame = glfwGetTime();
	Time.unscaledTime = currentFrame;
	Time.unscaledDeltaTime = currentFrame - lastFrame;
	Time.deltaTime = std::min(Time.unscaledDeltaTime, 1.f / 30.f) * Time.timeScale;
	Time.time += Time.deltaTime;
	Time.timeBuffer.add(Time.unscaledDeltaTime);
	lastFrame = currentFrame;
	Time.unscaledSmoothDeltaTime = Time.timeBuffer.getAverageValue();
	// eventsPollDone = true;
}

int frameCounter = 0;
#include "thread"
#include <tbb/tbb.h>
// #include <sched.h>

// void GLAPIENTRY
// MessageCallback(GLenum source,
// 				GLenum type,
// 				GLuint id,
// 				GLenum severity,
// 				GLsizei length,
// 				const GLchar *message,
// 				const void *userParam)
// {
// 	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
// 			(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
// 			type, severity, message);
// }

void dockspace()
{
	static bool open = true;
	static bool *p_open = &open;
	static bool opt_fullscreen = true;
	static bool opt_padding = false;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}
	else
	{
		dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	if (!opt_padding)
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", p_open, window_flags);
	if (!opt_padding)
		ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO &io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}
	else
	{
		// ShowDockingDisabledMessage();
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Options"))
		{
			// Disabling fullscreen would allow the window to be moved to the front of other windows,
			// which we can't undo at the moment without finer window depth/z control.
			ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
			ImGui::MenuItem("Padding", NULL, &opt_padding);
			ImGui::Separator();

			if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))
			{
				dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
			}
			if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))
			{
				dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
			}
			if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
			{
				dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
			}
			if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
			{
				dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
			}
			if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen))
			{
				dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
			}
			ImGui::Separator();

			if (ImGui::MenuItem("Close", NULL, false, p_open != NULL))
				*p_open = false;
			ImGui::EndMenu();
		}
		// HelpMarker(
		// 	"When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!"
		// 	"\n\n"
		// 	" > if io.ConfigDockingWithShift==false (default):"
		// 	"\n"
		// 	"   drag windows from title bar to dock"
		// 	"\n"
		// 	" > if io.ConfigDockingWithShift==true:"
		// 	"\n"
		// 	"   drag windows from anywhere and hold Shift to dock"
		// 	"\n\n"
		// 	"This demo app has nothing to do with it!"
		// 	"\n\n"
		// 	"This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)."
		// 	"\n\n"
		// 	"ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame."
		// 	"\n\n"
		// 	"(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)");

		ImGui::EndMenuBar();
	}

	ImGui::End();
}
tbb::concurrent_queue<glm::vec3> floating_origin;
atomic<bool> transformsBuffered;
void renderThreadFunc()
{
	// const size_t size = CPU_ALLOC_SIZE( concurrency::pinningObserver.ncpus );
	// cpu_set_t *target_mask = CPU_ALLOC( concurrency::pinningObserver.ncpus );
	// CPU_ZERO_S( size, target_mask );
	// CPU_SET_S( 0, size, target_mask );
	// CPU_SET_S( 1, size, target_mask );
	// const int err = sched_setaffinity( 0, size, target_mask );

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
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
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;	  // Enable docking

	// Setup style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	// ImGui::StyleColorsLight();

	auto font_default = io.Fonts->AddFontDefault();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!"); // Exceptionally add an extra assert here for people confused with initial dear imgui setup

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

	Shader matProgram("res/shaders/transform.comp");

	GLint glIntv;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &glIntv);
	cout << "max storage buffer bindings: " << glIntv << endl;

	glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &glIntv);
	cout << "max compute buffers: " << glIntv << endl;

	glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &glIntv);
	cout << "max buffer size: " << glIntv << endl;

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &glIntv);
	cout << "max compute work group count x: " << glIntv << endl;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &glIntv);
	cout << "max compute work group count y: " << glIntv << endl;
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &glIntv);
	cout << "max compute work group count z: " << glIntv << endl;

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &glIntv);
	cout << "max compute work group size x: " << glIntv << endl;

	GLint maxAtt = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAtt);
	cout << "max color attachements: " << maxAtt << endl;

	// // During init, enable debug output
	// glEnable(GL_DEBUG_OUTPUT);
	// glDebugMessageCallback(MessageCallback, 0);

	// shadowShader = new Shader("res/shaders/directional_shadow_map.vert", "res/shaders/directional_shadow_map.frag", false);
	// OmniShadowShader = new Shader("res/shaders/omni_shadow_map.vert", "res/shaders/omni_shadow_map.geom", "res/shaders/omni_shadow_map.frag", false);
	GPU_MATRIXES = new gpu_vector_proxy<matrix>();

	__RENDERERS_in = new gpu_vector<__renderer>();
	__RENDERERS_in->ownStorage();
	// __RENDERERS_keys_in = new gpu_vector<GLuint>();
	// __RENDERERS_keys_in->ownStorage();
	// __RENDERERS_out = new gpu_vector_proxy<__renderer>();
	// __RENDERERS_keys_out = new gpu_vector_proxy<GLuint>();

	__renderer_offsets = new gpu_vector<GLuint>();
	__renderer_offsets->ownStorage();
	__rendererMetas = new gpu_vector<__renderMeta>();
	__rendererMetas->ownStorage();
	initTransform();
	vector<glm::vec3> ughh(10);
	vector<glm::quat> ughh2(10);
	gpu_position_updates->bufferData(ughh);
	gpu_rotation_updates->bufferData(ughh2);
	gpu_scale_updates->bufferData(ughh);

	initParticles();
	particle_renderer::init();
	lighting::init();
	timer stopWatch;

	renderTexture colors;
	colors.scr_width = SCREEN_WIDTH;
	colors.scr_height = SCREEN_HEIGHT;
	colors.init();

	renderDone.store(true);
	renderThreadReady.store(true);

	// _atomics = new gpu_vector<uint>();
	_block_sums = new gpu_vector<GLuint>();
	_histo = new gpu_vector<GLuint>();

	sorter<__renderer> renderer_sorter("renderer", "struct renderer {\
	uint transform;\
	uint id;\
};",
									   "transform");

	while (true)
	{
		// log("render loop");
		// log(to_string(renderWork.size()));
		if (renderWork.size() > 0)
		{
			renderLock.lock();
			renderJob *rj = renderWork.front();
			renderWork.pop();
			switch (rj->type)
			{
			case renderNum::doFunc: //    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				renderLock.unlock();

				rj->work();

				if (rj->completed == 0)
				{
					rj->completed = 1;
					while (rj->completed != 2)
					{
						this_thread::sleep_for(1ns);
					}
				}

				break;
			case renderNum::render:
			{
				gpuTimer gt_;
				timer cpuTimer;
				stopWatch.start();
				// updateTiming();

				auto cameras = COMPONENT_LIST(_camera);

				gt_.start();
				cpuTimer.start();
				// buffer and allocate data
				// if (Transforms.density() > 0.5)
				// {
				// 	GPU_TRANSFORMS->bufferData(TRANSFORMS_TO_BUFFER);
				// }
				// else
				// {
				matProgram.use();

				GPU_TRANSFORMS->grow(Transforms.size());
				transformIds->bindData(6);
				GPU_TRANSFORMS->bindData(0);
				gpu_position_updates->bindData(8);
				gpu_rotation_updates->bindData(9);
				gpu_scale_updates->bindData(10);

				matProgram.setInt("stage", -2); // positions
				for (int i = 0; i < concurrency::numThreads; i++)
				{
					transformIds->bufferData(transformIdThreadcache[i][0]);
					// transformIds->bindData(6);

					gpu_position_updates->bufferData(positionsToBuffer[i]);
					// gpu_position_updates->bindData(8);

					matProgram.setUint("num", transformIdThreadcache[i][0].size());
					glDispatchCompute(transformIdThreadcache[i][0].size() / 64 + 1, 1, 1);
					glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
				}

				matProgram.setInt("stage", -3); // rotations
				for (int i = 0; i < concurrency::numThreads; i++)
				{
					transformIds->bufferData(transformIdThreadcache[i][1]);
					// transformIds->bindData(6);
					gpu_rotation_updates->bufferData(rotationsToBuffer[i]);
					// gpu_rotation_updates->bindData(9);

					matProgram.setUint("num", transformIdThreadcache[i][1].size());
					glDispatchCompute(transformIdThreadcache[i][1].size() / 64 + 1, 1, 1);
					glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
				}

				matProgram.setInt("stage", -4); // scales
				for (int i = 0; i < concurrency::numThreads; i++)
				{
					transformIds->bufferData(transformIdThreadcache[i][2]);
					// transformIds->bindData(6);
					gpu_scale_updates->bufferData(scalesToBuffer[i]);
					// gpu_scale_updates->bindData(10);

					matProgram.setUint("num", transformIdThreadcache[i][2].size());
					glDispatchCompute(transformIdThreadcache[i][2].size() / 64 + 1, 1, 1);
					glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
				}

				// transformIds->bufferData(transformIdsToBuffer);
				// GPU_TRANSFORMS_UPDATES->bufferData(transformsToBuffer);

				// matProgram.use();
				// // bind buffers
				// GPU_TRANSFORMS->bindData(0);
				// GPU_TRANSFORMS_UPDATES->bindData(7);

				// matProgram.setInt("stage", -1);
				// matProgram.setUint("num", transformsToBuffer.size());
				// glDispatchCompute(transformsToBuffer.size() / 64 + 1, 1, 1);
				// glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
				// }

				// //sort renderers
				// gt_.start();
				__RENDERERS_in->bufferData();
				// __RENDERERS_out->tryRealloc(__RENDERERS_in->size());
				// renderer_sorter.sort(__RENDERERS_in->size(),__RENDERERS_in,__RENDERERS_out);
				// appendStat("renderer sort", gt_.stop());

				transformsBuffered.store(true);
				appendStat("transforms buffer cpu", cpuTimer.stop());
				appendStat("transforms buffer", gt_.stop());

				// cpuTimer.start();
				uint emitterInitCount = emitterInits.size();
				prepParticles();

				// _camera::initPrepRender(matProgram);
				// appendStat("render init cpu", cpuTimer.stop());

				lightingManager::gpu_pointLights->bufferData();

				gt_.start();
				glm::vec3 fo;
				floating_origin.try_pop(fo);
				updateParticles(fo, emitterInitCount);
				appendStat("particles compute", gt_.stop());

				for (_camera &c : cameras->data.data)
				{
					gt_.start();
					cpuTimer.start();
					c.prepRender(matProgram);
					appendStat("matrix compute cpu", cpuTimer.stop());
					appendStat("matrix compute", gt_.stop());

					// sort particles
					timer t;
					t.start();
					if (!c.lockFrustum)
						particle_renderer::setCamCull(c.camInv, c.cullpos);
					particle_renderer::sortParticles(c.proj * c.rot * c.view, c.rot * c.view, mainCamPos, c.screen);
					appendStat("particles sort", t.stop());

					c.render();
				}

				///////////////////////// GUI ////////////////////////////////

				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

				ImGui::PushFont(font_default);

				// static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

				// ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
				// if (true)
				// {
				// 	ImGuiViewport* viewport = ImGui::GetMainViewport();
				// 	ImGui::SetNextWindowPos(viewport->GetWorkPos());
				// 	ImGui::SetNextWindowSize(viewport->GetWorkSize());
				// 	ImGui::SetNextWindowViewport(viewport->ID);
				// 	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				// 	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				// 	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				// 	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
				// }
				// else
				// {
				// 	dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
				// }

				// // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
				// // and handle the pass-thru hole, so we ask Begin() to not render a background.
				// if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
				// 	window_flags |= ImGuiWindowFlags_NoBackground;

				// // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
				// // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
				// // all active windows docked into it will lose their parent and become undocked.
				// // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
				// // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
				// if (!false)
				// 	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
				// ImGui::Begin("DockSpace Demo", 0, window_flags);
				// ImGui::End();

				dockspace();

				for (auto &i : gui::gui_windows)
				{
					i->render();
				}
				ImGui::PopFont();
				// Rendering
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
				ImGui::EndFrame();

				///////////////////////// GUI ////////////////////////////////
				renderLock.unlock();

				glfwSwapBuffers(window);

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
