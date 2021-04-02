#include "editor_layer.h"
#include <filesystem>
#include "Transform.h"
#include "particles/particles.h"

game_object *rootGameObject;
tbb::concurrent_queue<function<void()> *> mainThreadWork;
bool gameRunning = false;
inspectable *inspector = 0;
ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow; // | ImGuiTreeNodeFlags_OpenOnDoubleClick;
transform2 selected_transform = -1;
static unordered_map<int, bool> selected_transforms;

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
void initComponents(componentStorageBase *cl)
{
	for (int i = 0; i < cl->size(); ++i)
	{
		if (cl->getv(i))
		{
			cl->get(i)->init(i);
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
		// audioManager::save(oa);
		shaderManager::save(oa);
		modelManager::save(oa);
		saveEmitters(oa);
		oa << prototypeRegistry;
		assets::save(oa);
	}
	{
		OARCHIVE oa(ofs);
		game_object::serialize(oa, rootGameObject);
	}
	assets.close();
	ofs.close();
}
void rebuildGameObject(componentStorageBase *base, int i);

atomic<bool> stoppingGame;

void load_level(const char *filename) // assumes only renderer and terrain are started
{
	stoppingGame = true;
	lock_guard<mutex> lk(transformLock);
	rootGameObject->_destroy();

	// open the archive
	std::ifstream ifs(filename);
	std::ifstream assets("assets.ass");
	// std::ifstream ifsp("proto.lvl");
	if (assets::assets.size() == 0)
	{
		IARCHIVE ia(assets);
		// IARCHIVE ip(ifsp);
		ia >> working_file;
		// audioManager::load(ia);
		shaderManager::load(ia);
		modelManager::load(ia);
		loadEmitters(ia);
		ia >> prototypeRegistry;
		assets::load(ia);
	}
	if (string(filename) != "")
	{
		IARCHIVE ia(ifs);
		map<int, int> transform_map;
		game_object::deserialize(ia, transform_map);
	}
	rootGameObject = transform2(0)->gameObject();
	for (auto &i : ComponentRegistry.meta_types)
	{
		initComponents(i.second->getStorage());
	}
	stoppingGame = false;
}

// stringstream ss;

bool isGameRunning()
{
	return gameRunning;
}
// #define IAR boost::archive::binary_iarchive
// #define OAR boost::archive::binary_oarchive
void start_game() // assumes only renderer and terrain are started.
{
	mainThreadWork.push(new function<void()>([&]() {
		ofstream f(".temp");
		{
			// boost::archive::binary_oarchive oa(ss);
			OARCHIVE oa(f);
			game_object::serialize(oa, rootGameObject);
		}
		f.close();

		for (auto &i : ComponentRegistry.meta_types)
		{
			StartComponents(i.second->getStorage());
		}
	}));
	gameRunning = true;
}

void stop_game()
{
	gameRunning = false;
	inspector = 0;
	stoppingGame = true;
	mainThreadWork.push(new function<void()>([&]() {
		lock_guard<mutex> lk(transformLock);

		rootGameObject->_destroy();
		ifstream f(".temp");
		{
			// boost::archive::binary_iarchive ia(ss);
			IARCHIVE ia(f);
			map<int, int> transform_map;
			game_object::deserialize(ia, transform_map);
		}
		f.close();

		rootGameObject = transform2(0)->gameObject();
		for (auto &i : ComponentRegistry.meta_types)
		{
			initComponents(i.second->getStorage());
		}

		stoppingGame = false;

		// load_level(working_file.c_str());
	}));
}

void renderTransform(transform2 t, int &count)
{
	if (count > 10'000)
		return;
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
			renderTransform(i, ++count);
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
		for (const auto &entry : std::filesystem::directory_iterator(dir))
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
void dockspace(GLFWwindow *window, editor *m_editor)
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

	if ((ImGui::GetIO().KeysDown[GLFW_KEY_LEFT_CONTROL] && ImGui::IsKeyPressed(GLFW_KEY_O)))
	{
		openFile();
	}
	if ((ImGui::GetIO().KeysDown[GLFW_KEY_LEFT_CONTROL] && ImGui::GetIO().KeysDown[GLFW_KEY_LEFT_SHIFT] && ImGui::IsKeyPressed(GLFW_KEY_S)))
	{
		saveAsFile();
	}
	if ((ImGui::GetIO().KeysDown[GLFW_KEY_LEFT_CONTROL] && ImGui::IsKeyPressed(GLFW_KEY_S)))
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
		// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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
			// for (auto &i : assets::assets)
			// {
			// 	ImGui::BeginGroup();
			// 	ImGui::PushItemWidth(50);
			// 	ImGui::PushID(i.second->id);

			// 	char input[1024];
			// 	sprintf(input, i.second->name.c_str());
			// 	if (ImGui::InputText("", input, 1024, ImGuiInputTextFlags_None))
			// 		i.second->name = {input};
			// 	// ImVec2 sz{50,50};
			// 	// ImGui::Image(0,sz);
			// 	if (ImGui::Button(i.second->name.c_str(), {50, 50}))
			// 	{
			// 		// if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)){
			// 		inspector = i.second;
			// 	}
			// 	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
			// 	{
			// 		// cout << "right clicked button" << endl;
			// 		as = i.second;
			// 		open_asset = true;
			// 	}

			// 	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			// 	{
			// 		// Set payload to carry the index of our item (could be anything)
			// 		ImGui::SetDragDropPayload(i.second->type().c_str(), &i.second->id, sizeof(int));
			// 		ImGui::EndDragDropSource();
			// 	}

			// 	ImGui::PopID();
			// 	ImGui::PopItemWidth();

			// 	float last_button_x2 = ImGui::GetItemRectMax().x;
			// 	float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 50; // Expected position if next button was on same line
			// 	ImGui::EndGroup();
			// 	if (kjj++ + 1 < assets::assets.size() && next_button_x2 < window_visible_x2)
			// 		ImGui::SameLine();
			// }
			static auto renderAsset = [&](assets::asset *a) {
				ImGui::PushID(a->id);
				// ImGui::Text(s.second.get()->shader->vertexFile.c_str());
				ImGui::Button(a->name.c_str());
				// ImGui::InvisibleButton((s.second.get()->shader->vertexFile + "_inv").c_str(),{ImGui::GetFont()->FontSize,ImGui::GetFont()->FontSize * s.second.get()->shader->vertexFile.length()});
				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
				{
					inspector = a;
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
				{
					// cout << "right clicked button" << endl;
					as = a;
					open_asset = true;
				}

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
				{
					// Set payload to carry the index of our item (could be anything)
					ImGui::SetDragDropPayload(a->type().c_str(), &a->id, sizeof(int));
					ImGui::EndDragDropSource();
				}
				ImGui::PopID();
			};
			for (auto &s : shaderManager::shaders)
			{
				renderAsset(s.second.get());
			}
			for (auto &m : modelManager::models)
			{
				renderAsset(m.second.get());
			}
			for(auto &p : emitter_proto_assets){
				renderAsset(p.second);
			}
			bool open_rename = false;
			if (open_asset)
				ImGui::OpenPopup("asset_context");
			if (ImGui::BeginPopup("asset_context"))
			{
				ImGui::Separator();
				if (ImGui::Selectable("copy"))
				{
					as->copy();
				}
				if (ImGui::Selectable("delete"))
				{
				}
				if (ImGui::Selectable("rename"))
				{
					open_rename = true;
				}
				ImGui::EndPopup();
			}
			else if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
			{
				ImGui::OpenPopup("new_asset_context");
			}
			if (open_rename)
				ImGui::OpenPopup("rename_asset");
			if (ImGui::BeginPopup("rename_asset"))
			{
				renderEdit("", as->name);
				ImGui::EndPopup();
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
					emitter_prototype_ a = createEmitter("emitter " + to_string(emitter_proto_assets.size()));
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
			for (const auto &entry : std::filesystem::directory_iterator(path))
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
			// if(transformLock.try_lock()){

			if (!stoppingGame)
			{
				transformLock.lock();
				int count = 0;
				renderTransform(root2, count);
				transformLock.unlock();
			}
			// }
			ImGui::End();
		}

		// m_editor->update();
		bool using_gizmo = false;
		if (selected_transform.id != -1)
		{

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			float ww = ImGui::GetWindowWidth();
			float wy = ImGui::GetWindowHeight();
			ImGuizmo::SetRect(0.0f, 0.0f, ww, wy);
			glm::mat4 view = m_editor->c.rot * m_editor->c.view;
			// mat4 view = COMPONENT_LIST(_camera)->get(0)->view;
			glm::mat4 proj = m_editor->c.proj;
			// mat4 proj = glm::perspective(COMPONENT_LIST(_camera)->get(0)->fov, (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.01f, 1e6f);
			glm::mat4 trans = selected_transform.getModel();
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
				glm::vec3 pos;
				glm::vec3 scale;
				glm::quat rot;
				glm::vec3 skew;
				glm::vec4 pers;
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
			ImGui::Text(string{"fps: " + to_string(1.f / Time.unscaledSmoothDeltaTime)}.c_str());
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
			glm::vec2 sz_2 = {sz.x, sz.y};
			sz_2 /= 2.f;

			camera &c = m_editor->c;
			glm::mat3 per = c.getProjection();

			glm::vec3 p = m_editor->position;
			glm::vec3 d = c.screenPosToRay({mp.x, mp.y});

			mainThreadWork.push(new function<void()>([=]() {
				transformLock.lock();
				transform2 r = renderRaycast(p, d);
				if (r.id != -1)
				{
					inspector = r->gameObject();
					selected_transform = r;
					selected_transforms.clear();
					selected_transforms[r.id] = true;
				}
				transformLock.unlock();
			}));
		}
	}
	else
	{
		if (ImGui::Begin("info"))
		{
			ImGui::Text(string{"fps: " + to_string(1.f / Time.unscaledDeltaTime)}.c_str());
			ImGui::Text(string{"entities: " + FormatWithCommas(Transforms.getCount())}.c_str());
			// ImGui::Text(string{"particles: " + FormatWithCommas(getParticleCount())}.c_str());
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
