#include "renderthread.h"
#include <algorithm>
#include <imgui/guizmo/ImGuizmo.h>
#include "terrain.h"
// #include <imgui/guizmo/ImGradient.h>
// #include "physics.h"
namespace fs = std::filesystem;

using namespace std;

GLFWwindow *window;
GLdouble lastFrame = 0;
Shader *shadowShader;
Shader *OmniShadowShader;

bool hideMouse = true;
atomic<bool> renderDone(false);
atomic<bool> renderThreadReady(false);
bool recieveMouse = true;

editor *m_editor;

bool gameRunning = false;
inspectable *inspector = 0;
ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow; // | ImGuiTreeNodeFlags_OpenOnDoubleClick;
transform2 selected_transform = -1;
static unordered_map<int, bool> selected_transforms;

void editor::translate(glm::vec3 v)
{
	this->position += this->rotation * v;
}
void editor::rotate(glm::vec3 axis, float radians)
{
	this->rotation = glm::rotate(this->rotation, radians, axis);
}

void editor::update()
{

	if (Input.Mouse.getButton(GLFW_MOUSE_BUTTON_2))
	{

		translate(glm::vec3(1, 0, 0) * (float)(Input.getKey(GLFW_KEY_A) - Input.getKey(GLFW_KEY_D)) * Time.deltaTime * speed);
		translate(glm::vec3(0, 0, 1) * (float)(Input.getKey(GLFW_KEY_W) - Input.getKey(GLFW_KEY_S)) * Time.deltaTime * speed);
		translate(glm::vec3(0, 1, 0) * (float)(Input.getKey(GLFW_KEY_SPACE) - Input.getKey(GLFW_KEY_LEFT_SHIFT)) * Time.deltaTime * speed);
		// transform->rotate(glm::vec3(0, 0, 1), (float)(Input.getKey(GLFW_KEY_Q) - Input.getKey(GLFW_KEY_E)) * -Time.deltaTime);
		rotate(vec3(0, 1, 0), Input.Mouse.getX() * Time.unscaledDeltaTime * c.fov / radians(80.f) * -0.4f);
		rotate(vec3(1, 0, 0), Input.Mouse.getY() * Time.unscaledDeltaTime * c.fov / radians(80.f) * -0.4f);

		rotation = quatLookAtLH(rotation * vec3(0, 0, 1), vec3(0, 1, 0));

		c.fov -= Input.Mouse.getScroll() * radians(5.f);
		c.fov = glm::clamp(c.fov, radians(5.f), radians(80.f));

		if (Input.getKeyDown(GLFW_KEY_R))
		{
			speed *= 2;
		}
		if (Input.getKeyDown(GLFW_KEY_F))
		{
			speed /= 2;
		}
	}
	c.update(position, rotation);
}
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
			ImGui::GetIO().MouseDown[button] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			Input.Mouse.mouseButtons[button] = false;
			ImGui::GetIO().MouseDown[button] = false;
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

game_object *rootGameObject;
tbb::concurrent_queue<function<void()> *> mainThreadWork;

void StartComponents(componentStorageBase *cl)
{
	for (int i = 0; i < cl->size(); ++i)
	{
		if (cl->getv(i))
		{
			cl->get(i)->onStart();
		}
	}
}

void DestroyComponents(componentStorageBase *cl)
{
	for (int i = 0; i < cl->size(); ++i)
	{
		if (cl->getv(i))
		{
			cl->get(i)->onDestroy();
		}
	}
}

void save_level(const char *filename)
{
	// make an archive
	std::ofstream ofs(filename);
	std::ofstream assets("assets.ass");
	// std::ofstream oproto("proto.lvl");
	{
		OARCHIVE oa(assets);
		// OARCHIVE op(oproto);
		oa << working_file;
		audioManager::save(oa);
		shaderManager::save(oa);
		modelManager::save(oa);
		saveEmitters(oa);
		oa << prototypeRegistry;
		assets::save(oa);
	}
	{
		OARCHIVE oa(ofs);
		// lightingManager::save(oa);
		saveTransforms(oa);
		oa << ComponentRegistry;
	}
	assets.close();
	ofs.close();
}

void load_level(const char *filename) // assumes only renderer and terrain are started
{

	DestroyComponents(COMPONENT_LIST(_renderer));
	DestroyComponents(COMPONENT_LIST(terrain));

	for (auto &child : rootGameObject->transform->getChildren())
	{
		child->gameObject()->destroy();
	}
	tbb::parallel_for_each(toDestroyGameObjects.range(), [](game_object *g) { g->_destroy(); });
	toDestroyGameObjects.clear();

	// for(auto& g : toDestroy){
	// 	g->_destroy();
	// }
	// tbb::parallel_for_each(toDestroy.range(), [](game_object *g) { g->_destroy(); });
	// toDestroy.clear();
	ComponentRegistry.clear();

	// open the archive
	std::ifstream ifs(filename);
	std::ifstream assets("assets.ass");
	// std::ifstream ifsp("proto.lvl");
	if (assets::assets.size() == 0)
	{
		IARCHIVE ia(assets);
		// IARCHIVE ip(ifsp);
		ia >> working_file;
		audioManager::load(ia);
		shaderManager::load(ia);
		modelManager::load(ia);
		loadEmitters(ia);
		ia >> prototypeRegistry;
		assets::load(ia);
	}
	if (string(filename) != "")
	{
		IARCHIVE ia(ifs);
		// lightingManager::load(ia);
		loadTransforms(ia);
		ia >> ComponentRegistry;
	}
	for (auto &i : ComponentRegistry.components)
	{
		for (int j = 0; j < i.second->size(); j++)
		{
			if (i.second->getv(j))
			{
				rebuildGameObject(i.second, j);
				i.second->get(j)->init();
			}
		}
	}

	StartComponents(COMPONENT_LIST(_renderer));
	StartComponents(COMPONENT_LIST(terrain));
	// for (int i = 0; i < cl->size(); ++i)
	// {
	// 	if (cl->getv(i))
	// 	{
	// 		cl->get(i)->onStart();
	// 	}
	// }
	// for (auto &i : ComponentRegistry.components)
	// {
	// 	for (int j = 0; j < i.second->size(); j++)
	// 	{
	// 		if (i.second->getv(j))
	// 		{
	// 			i.second->get(j)->onStart();
	// 		}
	// 	}
	// }
}

stringstream ss;

bool isGameRunning()
{
	return gameRunning;
}
// #define IARCHIVE boost::archive::binary_iarchive
// #define OARCHIVE boost::archive::binary_oarchive
void start_game() // assumes only renderer and terrain are started.
{

	mainThreadWork.push(new function<void()>([&]() {
		{
			// boost::archive::binary_oarchive oa(ss);
			OARCHIVE oa(ss);
			// lightingManager::save(oa);
			saveTransforms(oa);
			oa << ComponentRegistry;
		}

		for (auto &i : ComponentRegistry.components)
		{
			if (i.first != typeid(_renderer).hash_code() && i.first != typeid(terrain).hash_code())
				StartComponents(i.second);
		}
	}));
	gameRunning = true;
}

void stop_game()
{
	gameRunning = false;
	inspector = 0;
	mainThreadWork.push(new function<void()>([&]() {
		for (auto &child : rootGameObject->transform->getChildren())
		{
			child->gameObject()->destroy();
		}
		tbb::parallel_for_each(toDestroyGameObjects.range(), [](game_object *g) { g->_destroy(); });
		toDestroyGameObjects.clear();
		ComponentRegistry.clear();
		{
			// boost::archive::binary_iarchive ia(ss);
			IARCHIVE ia(ss);
			// lightingManager::load(ia);
			loadTransforms(ia);
			ia >> ComponentRegistry;
		}
		for (auto &i : ComponentRegistry.components)
		{
			i.second->s();
			// for (int j = 0; j < i.second->size(); j++)
			// {
			// 	// if (i.second->getv(j))
			// 	// {
			// 	// 	rebuildGameObject(i.second, j);
			// 	// 	i.second->get(j)->init();
			// 	// }
			// }
		}
		ss.clear();
		StartComponents(COMPONENT_LIST(_renderer));
		StartComponents(COMPONENT_LIST(terrain));

		// load_level(working_file.c_str());
	}));
}

void renderTransform(transform2 t)
{
	ImGuiTreeNodeFlags flags = base_flags;
	if (selected_transforms[t.id])
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (t.getChildren().size() == 0)
		flags |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf;
	bool open;
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (t.name() == "")
		open = ImGui::TreeNodeEx(("game object " + to_string(t.id)).c_str(), flags);
	else
		open = ImGui::TreeNodeEx(t.name().c_str(), flags);

	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		ImGui::SetDragDropPayload("TRANSFORM_DRAG_AND_DROP", &t.id, sizeof(int));
		ImGui::EndDragDropSource();
	}

	if (ImGui::IsMouseReleased(0) && ImGui::IsItemHovered()) // mouseUp(clicked, t.id)) // mouse up
	{
		if (Input.getKey(GLFW_KEY_LEFT_CONTROL))
			selected_transforms[t.id] = true;
		else
		{
			selected_transforms.clear();
			selected_transforms[t.id] = true;
		}
		selected_transform.id = t.id;
		inspector = t->gameObject();
	}

	if (ImGui::IsItemClicked(1))
		ImGui::OpenPopup("game_object_context");
	if (ImGui::BeginPopup("game_object_context"))
	{
		// ImGui::Text("collider type");
		ImGui::Separator();
		if (ImGui::Selectable("copy"))
		{
			_instantiate(*t->gameObject());
		}
		if (ImGui::Selectable("delete"))
		{
			if (inspector == t->gameObject())
				inspector = 0;
			t->gameObject()->destroy();
		}
		if (ImGui::Selectable("new game object"))
		{
			_instantiate();
		}
		ImGui::EndPopup();
	}

	if (open)
	{
		for (auto &i : t.getChildren())
			renderTransform(i);
		ImGui::TreePop();
	}
}

string working_file = "";
hash<string> hasher;
// string file_dragged;
// string *file_dragged_ptr;

void renderFile(string file)
{
	static unordered_map<size_t, bool> file_slected;
	string p = file;
	size_t hash = hasher(file);
	p = p.substr(p.find_last_of('/') + 1);
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf;
	if (file_slected[hash])
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (ImGui::TreeNodeEx(p.c_str(), flags))
	{
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload(("FILE_DRAG_AND_DROP" + p.substr(p.find_first_of('.'))).c_str(), file.substr(2).c_str(), sizeof(char) * file.size() - 1);
			ImGui::EndDragDropSource();
		}

		if (ImGui::IsMouseReleased(0) && ImGui::IsItemHovered()) // mouseUp(clicked, t.id)) // mouse up
		{
			if (Input.getKey(GLFW_KEY_LEFT_CONTROL))
				file_slected[hash] = true;
			else
			{
				file_slected.clear();
				file_slected[hash] = true;
			}
		}
		ImGui::TreePop();
	}
}

void nav_f(string dir)
{
	if (ImGui::TreeNode(dir.substr(dir.find_last_of('/') + 1).c_str()))
	{
		vector<string> dirs;
		vector<string> files;
		for (const auto &entry : fs::directory_iterator(dir))
		{
			if (entry.is_directory())
			{
				dirs.push_back(entry.path());
			}
			else
			{
				files.push_back(entry.path());
			}
		}
		sort(dirs.begin(), dirs.end());
		sort(files.begin(), files.end());

		for (const auto &dir : dirs)
		{
			nav_f(dir);
		}
		for (const auto &file : files)
		{
			renderFile(file);
		}
		ImGui::TreePop();
	}
}

void openFile()
{
	inspector = 0;
	char file[1024] = {};
	FILE *f = popen("zenity --file-selection --file-filter=*.lvl", "r");
	fgets(file, 1024, f);
	string fi(file);
	fi = fi.substr(0, fi.size() - 1);
	working_file = fi;
	if (fi == "")
		cout << "cancelled load" << endl;
	else
	{
		mainThreadWork.push(new function<void()>([=]() {
			load_level(fi.c_str());
		}));
		cout << "loaded: " << file << endl;
	}
}
void saveAsFile()
{
	char file[1024];
	FILE *f = popen("zenity --file-selection --save  --file-filter=*.lvl", "r");
	fgets(file, 1024, f);
	string fi(file);
	fi = fi.substr(0, fi.size() - 1);
	if (fi == "")
		cout << "cancelled save" << endl;
	else
	{
		mainThreadWork.push(new function<void()>([=]() {
			save_level(fi.c_str());
		}));

		working_file = fi;
		cout << "saved: " << file << endl;
	}
}
void saveFile()
{
	mainThreadWork.push(new function<void()>([=]() {
		save_level(working_file.c_str());
	}));
	cout << "saved: " << working_file << endl;
}
mutex transformLock;
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
	// if (opt_fullscreen)
	// {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetWorkPos());
	ImGui::SetNextWindowSize(viewport->GetWorkSize());
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	// }
	// else
	// {
	// 	dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
	// }

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

	if ((ImGui::GetIO().KeysDown[GLFW_KEY_LEFT_CONTROL] && ImGui::GetIO().KeysDown[GLFW_KEY_O]))
	{
		openFile();
	}
	if ((ImGui::GetIO().KeysDown[GLFW_KEY_LEFT_CONTROL] && ImGui::GetIO().KeysDown[GLFW_KEY_LEFT_SHIFT] && ImGui::GetIO().KeysDown[GLFW_KEY_S]))
	{
		saveAsFile();
	}
	if ((ImGui::GetIO().KeysDown[GLFW_KEY_LEFT_CONTROL] && ImGui::GetIO().KeysDown[GLFW_KEY_S]))
	{
		saveFile();
	}
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{

			if (ImGui::MenuItem("Open", NULL))
			{
				openFile();
			}
			if (ImGui::MenuItem("save as", NULL))
			{
				saveAsFile();
			}
			if (ImGui::MenuItem("save", NULL))
			{
				saveFile();
			}
			// ImGui::Separator();

			// if (ImGui::MenuItem("Close", NULL, false, p_open != NULL))
			// 	*p_open = false;
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	if (!isGameRunning())
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		bool editor_hov = false;
		if (ImGui::Begin("Assets"))
		{
			editor_hov |= ImGui::IsWindowHovered();
			static assets::asset *as;
			ImGuiStyle &style = ImGui::GetStyle();
			int buttons_count = 20;
			float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

			int kjj = 0;
			bool open_asset = false;
			for (auto &i : assets::assets)
			{
				ImGui::BeginGroup();
				ImGui::PushItemWidth(50);
				ImGui::PushID(i.second->id);

				char input[1024];
				sprintf(input, i.second->name.c_str());
				if (ImGui::InputText("", input, 1024, ImGuiInputTextFlags_None))
					i.second->name = {input};
				// ImVec2 sz{50,50};
				// ImGui::Image(0,sz);
				if (ImGui::Button(i.second->name.c_str(), {50, 50}))
				{
					// if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)){
					inspector = i.second;
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
				{
					// cout << "right clicked button" << endl;
					as = i.second;
					open_asset = true;
				}

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
				{
					// Set payload to carry the index of our item (could be anything)
					ImGui::SetDragDropPayload(i.second->type().c_str(), &i.second->id, sizeof(int));
					ImGui::EndDragDropSource();
				}

				ImGui::PopID();
				ImGui::PopItemWidth();

				float last_button_x2 = ImGui::GetItemRectMax().x;
				float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 50; // Expected position if next button was on same line
				ImGui::EndGroup();
				if (kjj++ + 1 < assets::assets.size() && next_button_x2 < window_visible_x2)
					ImGui::SameLine();
			}
			if (open_asset)
				ImGui::OpenPopup("asset_context");
			if (ImGui::BeginPopup("asset_context"))
			{
				// ImGui::Text("collider type");
				ImGui::Separator();
				if (ImGui::Selectable("copy"))
				{
					as->copy();
					// new game_object(*t->gameObject());
				}
				if (ImGui::Selectable("delete"))
				{
					// if (inspector == t->gameObject())
					// 	inspector = 0;
					// t->gameObject()->destroy();
				}
				ImGui::EndPopup();
			}
			else if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
			{
				ImGui::OpenPopup("new_asset_context");
			}
			if (ImGui::BeginPopup("new_asset_context"))
			{
				// ImGui::Text("collider type");
				ImGui::Separator();
				if (ImGui::Selectable("new model"))
				{
					// new game_object();
				}
				if (ImGui::Selectable("new emitter"))
				{
					// new game_object();
				}
				if (ImGui::Selectable("new shader"))
				{
					// new game_object();
				}
				if (ImGui::Selectable("new prototype"))
				{
					// new game_object();
				}
				ImGui::EndPopup();
			}

			ImGui::End();
		}

		if (ImGui::Begin("Files"))
		{
			editor_hov |= ImGui::IsWindowHovered();
			std::string path = ".";
			// if (ImGui::TreeNode(dir.substr(dir.find_last_of('/')).c_str()))
			// {

			vector<string> dirs;
			vector<string> files;
			for (const auto &entry : fs::directory_iterator(path))
			{
				if (entry.is_directory())
				{
					dirs.push_back(entry.path());
				}
				else
				{
					files.push_back(entry.path());
				}
			}
			sort(dirs.begin(), dirs.end());
			sort(files.begin(), files.end());

			for (const auto &dir : dirs)
			{
				nav_f(dir);
			}
			for (const auto &file : files)
			{

				renderFile(file);
			}

			// 	ImGui::TreePop();
			// }
			ImGui::End();
		}
		if (ImGui::Begin("Inspector"))
		{
			editor_hov |= ImGui::IsWindowHovered();
			if (inspector)
				inspector->inspect();
			ImGui::End();
		}
		if (ImGui::Begin("Hierarchy"))
		{
			editor_hov |= ImGui::IsWindowHovered();
			transformLock.lock();
			renderTransform(root2);
			transformLock.unlock();
			ImGui::End();
		}

		m_editor->update();
		bool using_gizmo = false;
		if (selected_transform.id != -1)
		{

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			float ww = ImGui::GetWindowWidth();
			float wy = ImGui::GetWindowHeight();
			ImGuizmo::SetRect(0.0f, 0.0f, ww, wy);
			mat4 view = m_editor->c.rot * m_editor->c.view;
			// mat4 view = COMPONENT_LIST(_camera)->get(0)->view;
			mat4 proj = m_editor->c.proj;
			// mat4 proj = glm::perspective(COMPONENT_LIST(_camera)->get(0)->fov, (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.01f, 1e6f);
			mat4 trans = selected_transform.getModel();
			static auto guizmo_mode = ImGuizmo::LOCAL;
			static auto guizmo_transform = ImGuizmo::OPERATION::TRANSLATE;
			if (!ImGui::IsMouseDown(1))
			{
				if (ImGui::IsKeyPressed(GLFW_KEY_T))
					guizmo_transform = ImGuizmo::OPERATION::TRANSLATE;
				if (ImGui::IsKeyPressed(GLFW_KEY_R))
					guizmo_transform = ImGuizmo::OPERATION::ROTATE;
				if (ImGui::IsKeyPressed(GLFW_KEY_S))
					guizmo_transform = ImGuizmo::OPERATION::SCALE;
				if (ImGui::IsKeyPressed(GLFW_KEY_W))
					guizmo_mode = ImGuizmo::WORLD;
				if (ImGui::IsKeyPressed(GLFW_KEY_L))
					guizmo_mode = ImGuizmo::LOCAL;
			}

			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), guizmo_transform, guizmo_mode, glm::value_ptr(trans));

			if (ImGuizmo::IsUsing())
			{
				vec3 pos;
				vec3 scale;
				quat rot;
				vec3 skew;
				vec4 pers;
				// ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(trans),pos,)
				glm::decompose(trans, scale, rot, pos, skew, pers);

				selected_transform.setPosition(pos);
				selected_transform.setRotation(rot);
				selected_transform.setScale(scale);
				using_gizmo = true;
			}
		}

		if (ImGui::Begin("info"))
		{
			editor_hov |= ImGui::IsWindowHovered();
			ImGui::Text(string{"fps: " + to_string(1.f / Time.unscaledDeltaTime)}.c_str());
			ImGui::Text(string{"entities: " + FormatWithCommas(Transforms.getCount())}.c_str());
			ImGui::Text(string{"particles: " + FormatWithCommas(getParticleCount())}.c_str());
			if (!isGameRunning() && ImGui::Button("play"))
			{
				start_game();
			}
			else if (isGameRunning() && ImGui::Button("stop"))
			{
				stop_game();
			}
			ImGui::End();
		}

		if (ImGui::IsMouseClicked(0) && !editor_hov && !using_gizmo)
		{
			ImVec2 mp = ImGui::GetMousePos();
			ImVec2 sz = ImGui::GetWindowViewport()->Size;
			cout << "mp: " << mp.x << "," << mp.y << " sz:" << sz.x << "," << sz.y << endl;
			vec2 sz_2 = {sz.x, sz.y};
			sz_2 /= 2.f;

			camera &c = m_editor->c;
			mat3 per = c.getProjection();

			vec3 p = m_editor->position;
			vec3 d = c.screenPosToRay({mp.x, mp.y});

			mainThreadWork.push(new function<void()>([=]() {
				transform2 r = renderRaycast(p, d);
				if (r.id != -1)
				{
					inspector = r->gameObject();
					selected_transform = r;
					selected_transforms.clear();
					selected_transforms[r.id] = true;
				}
			}));
		}
	}
	else
	{
		if (ImGui::Begin("info"))
		{
			ImGui::Text(string{"fps: " + to_string(1.f / Time.unscaledDeltaTime)}.c_str());
			ImGui::Text(string{"entities: " + FormatWithCommas(Transforms.getCount())}.c_str());
			ImGui::Text(string{"particles: " + FormatWithCommas(getParticleCount())}.c_str());
			if (!isGameRunning() && ImGui::Button("play"))
			{
				start_game();
			}
			else if (isGameRunning() && (ImGui::Button("stop") || ImGui::GetIO().KeysDown[GLFW_KEY_ESCAPE]))
			{
				stop_game();
			}
			ImGui::End();
		}
	}
	ImGui::End();
}

void printData()
{

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
}
void enabelDebug()
{

	// During init, enable debug output
	// glEnable(GL_DEBUG_OUTPUT);
	// glDebugMessageCallback(MessageCallback, 0);
}

void bindWindowCallbacks(GLFWwindow *window)
{
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetWindowCloseCallback(window, window_close_callback);
}

void initiliazeStuff()
{
	// shadowShader = new Shader("res/shaders/directional_shadow_map.vert", "res/shaders/directional_shadow_map.frag", false);
	// OmniShadowShader = new Shader("res/shaders/omni_shadow_map.vert", "res/shaders/omni_shadow_map.geom", "res/shaders/omni_shadow_map.frag", false);
	GPU_MATRIXES = new gpu_vector_proxy<matrix>();

	__RENDERERS_in = new gpu_vector<__renderer>();
	__RENDERERS_in->ownStorage();

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

	renderTexture colors;
	colors.scr_width = SCREEN_WIDTH;
	colors.scr_height = SCREEN_HEIGHT;
	colors.init();

	renderDone.store(true);
	renderThreadReady.store(true);

	// _atomics = new gpu_vector<uint>();
	_block_sums = new gpu_vector<GLuint>();
	_histo = new gpu_vector<GLuint>();
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

	///////////////////////// GUI ////////////////////////////////

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

	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	if (hideMouse)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	bindWindowCallbacks(window);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	if (glewInit() != GLEW_OK)
	{
		cout << "failed to initialize GLEW" << endl;
		throw EXIT_FAILURE;
	}
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	printData();

	initiliazeStuff();
	timer stopWatch;

	Shader matProgram("res/shaders/transform.comp");
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
					gpu_position_updates->bufferData(positionsToBuffer[i]);
					matProgram.setUint("num", transformIdThreadcache[i][0].size());
					glDispatchCompute(transformIdThreadcache[i][0].size() / 64 + 1, 1, 1);
					glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
				}

				matProgram.setInt("stage", -3); // rotations
				for (int i = 0; i < concurrency::numThreads; i++)
				{
					transformIds->bufferData(transformIdThreadcache[i][1]);
					gpu_rotation_updates->bufferData(rotationsToBuffer[i]);
					matProgram.setUint("num", transformIdThreadcache[i][1].size());
					glDispatchCompute(transformIdThreadcache[i][1].size() / 64 + 1, 1, 1);
					glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
				}

				matProgram.setInt("stage", -4); // scales
				for (int i = 0; i < concurrency::numThreads; i++)
				{
					transformIds->bufferData(transformIdThreadcache[i][2]);
					gpu_scale_updates->bufferData(scalesToBuffer[i]);
					matProgram.setUint("num", transformIdThreadcache[i][2].size());
					glDispatchCompute(transformIdThreadcache[i][2].size() / 64 + 1, 1, 1);
					glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
				}
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
				lightingManager::gpu_pointLights->bufferData();

				gt_.start();
				glm::vec3 fo;
				floating_origin.try_pop(fo);
				updateParticles(fo, emitterInitCount);
				appendStat("particles compute", gt_.stop());

				if (isGameRunning())
				{
					for (auto &c : cameras->_data)
					{
						c.second.c->prepRender(matProgram);
						c.second.c->render();
					}
				}
				else
				{
					if (m_editor)
					{
						m_editor->c.prepRender(matProgram);
						m_editor->c.render();
					}
				}

				///////////////////////// GUI ////////////////////////////////

				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();
				ImGuizmo::BeginFrame();

				ImGui::PushFont(font_default);

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
