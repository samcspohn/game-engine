#include "Transform.h"
#include "concurrency.h"
#include <fstream>
#include "game_object.h"

using namespace std;

mutex gameLock;
glm::vec3 vec3forward(0, 0, 1);
glm::vec3 vec3up(0, 1, 0);
glm::vec3 vec3right(1, 0, 0);
glm::vec3 mainCamPos;
glm::vec3 mainCamUp;
glm::vec3 MainCamForward;

atomic<int> GPU_TRANSFORMS_UPDATES_itr;
deque_heap<_transform> TRANSFORMS;
vector<_transform> TRANSFORMS_TO_BUFFER;
gpu_vector_proxy<_transform> *GPU_TRANSFORMS;
gpu_vector_proxy<GLint> *transformIds;
// gpu_vector_proxy<_transform>* GPU_TRANSFORMS_UPDATES;
gpu_vector_proxy<glm::vec3> *gpu_position_updates;
gpu_vector_proxy<glm::quat> *gpu_rotation_updates;
gpu_vector_proxy<glm::vec3> *gpu_scale_updates;

_Transforms Transforms;
transform2 root2;

transform2::transform2() {}

void transform2::_init()
{
	// gameLock.lock();
	// _T = TRANSFORMS._new();
	// gameLock.unlock();
	// parent = 0;
	root2.adopt(*this);
}
void transform2::init(game_object *g)
{
	Transforms.meta[id].gameObject = g;
	_init();
}

void transform2::init(transform2 other, game_object *go)
{
	Transforms.meta[id].gameObject = go;
	other.getParent().adopt(*this);
	Transforms.positions[id] = other.getPosition();
	Transforms.rotations[id] = other.getRotation();
	Transforms.scales[id] = other.getScale();
}

void transform2::init(transform2 &other)
{
}
transform2::~transform2() {}

transform2::transform2(int i) : id(i) {}
// transform2 transform2::operator=(const transform2& t) {
// 	this->_T = t._T;
// 	this->gameObject = t.gameObject;
// 	this->children = t.children;
// 	this->parent = t.parent;
// 	return *this;
// }

_transform transform2::getTransform()
{
	_transform t;
	t.position = getPosition();
	t.rotation = getRotation();
	t.scale = getScale();
	return t;
}
string& transform2::name(){
	return Transforms.meta[id].name;
}

void transform2::lookat(glm::vec3 direction, glm::vec3 up)
{
	Transforms.updates[id].rot = true;
	Transforms.rotations[id] = quatLookAtLH(direction, up);
}
glm::vec3 transform2::forward()
{
	return glm::normalize(Transforms.rotations[id] * glm::vec3(0.0f, 0.0f, 1.0f));
}
glm::vec3 transform2::right()
{
	return glm::normalize(Transforms.rotations[id] * glm::vec3(1.0f, 0.0f, 0.0f));
}
glm::vec3 transform2::up()
{
	return glm::normalize(Transforms.rotations[id] * glm::vec3(0.0f, 1.0f, 0.0f));
}
glm::mat4 transform2::getModel()
{
	return (glm::translate(Transforms.positions[id]) * glm::toMat4(Transforms.rotations[id]) * glm::scale(Transforms.scales[id]));
}
glm::vec3 transform2::getScale()
{
	return Transforms.scales[id];
}
void transform2::setScale(glm::vec3 scale)
{
	Transforms.updates[id].scl = true;
	this->scale(scale / Transforms.scales[id]);
}
glm::vec3 transform2::getPosition()
{
	return Transforms.positions[id];
}
void transform2::setPosition(glm::vec3 pos)
{
	// m.lock();
	Transforms.updates[id].pos = true;
	for (transform2 c : Transforms.meta[id].children)
		c->translate(pos - Transforms.positions[id], glm::quat());
	Transforms.positions[id] = pos;
	// m.unlock();
}
glm::quat transform2::getRotation()
{
	return Transforms.rotations[id];
}


void setRotationChild(transform2 tc, glm::quat& rot, glm::vec3& pos){
	#define rotat Transforms.rotations[tc.id]
	#define posi Transforms.positions[tc.id]

	posi = pos + rot * (posi - pos);
	rotat = rot * rotat;
	Transforms.updates[tc.id].pos = true;
	Transforms.updates[tc.id].rot = true;
	for(transform2 t : tc->getChildren()){
		setRotationChild(t,rot, pos);
	}
	
	#undef posi
	#undef rotat
}

void transform2::setRotation(glm::quat r)
{
	Transforms.updates[id].rot = true;
	glm::quat rot = r * glm::inverse(Transforms.rotations[id]);
	Transforms.rotations[id] = r;

	for(transform2 t : this->getChildren()){
		setRotationChild(t,rot, Transforms.positions[id]);
	}

}
list<transform2> &transform2::getChildren()
{
	return Transforms.meta[id].children;
}
transform2 transform2::getParent()
{
	return Transforms.meta[id].parent;
}

void transform2::adopt(transform2 transform)
{
	if (Transforms.meta[transform.id].parent.id == this->id)
		return;
	transform->orphan();
	Transforms.meta[transform.id].parent.id = this->id;
	Transforms.meta[transform.id].m.lock();
	Transforms.meta[id].children.push_back(transform);
	Transforms.meta[transform.id].childId = (--Transforms.meta[id].children.end());
	Transforms.meta[transform.id].m.unlock();
}

game_object *transform2::gameObject()
{
	return Transforms.meta[id].gameObject;
}

void transform2::setGameObject(game_object *g)
{
	Transforms.meta[id].gameObject = g;
}
void transform2::_destroy()
{
	orphan();
	Transforms._delete(*this);
	// if (enabled) {
	// 	TRANSFORMS._delete(_T);
	// }
	// else
	// {
	// 	STATIC_TRANSFORMS._delete(_T);
	// }
	// if (enabled)
	// 	--transforms_enabled;
	// delete this;
}

void transform2::move(glm::vec3 movement, bool hasChildren)
{
	Transforms.updates[id].pos = true;
	Transforms.positions[id] += movement;
	if (hasChildren)
	{
		for (auto a : Transforms.meta[id].children)
			a->translate(movement, glm::quat(1, 0, 0, 0));
	}
}
void transform2::translate(glm::vec3 translation)
{
	// m.lock();
	Transforms.updates[id].pos = true;
	Transforms.positions[id] += Transforms.rotations[id] * translation;
	for (auto a : Transforms.meta[id].children)
		a->translate(translation, Transforms.rotations[id]);
	// m.unlock();
}
void transform2::translate(glm::vec3 translation, glm::quat r)
{
	// m.lock();
	Transforms.updates[id].pos = true;
	Transforms.positions[id] += r * translation;
	for (transform2 a : Transforms.meta[id].children)
		a->translate(translation, r);
	// m.unlock();
}
void transform2::scale(glm::vec3 scale)
{
	// m.lock();
	Transforms.updates[id].scl = true;
	Transforms.scales[id] *= scale;
	for (transform2 a : Transforms.meta[id].children)
		a->scaleChild(Transforms.positions[id], scale);
	// m.unlock();
}
void transform2::scaleChild(glm::vec3 pos, glm::vec3 scale)
{
	// m.lock();
	Transforms.updates[id].scl = true;
	Transforms.positions[id] = (Transforms.positions[id] - pos) * scale + pos;
	Transforms.scales[id] *= scale;
	for (transform2 a : Transforms.meta[id].children)
		a->scaleChild(pos, scale);
	// m.unlock();
}
void transform2::rotate(glm::vec3 axis, float radians)
{
	// m.lock();
	Transforms.updates[id].rot = true;
	Transforms.rotations[id] = glm::rotate(Transforms.rotations[id], radians, axis);
	for (transform2 a : Transforms.meta[id].children)
		a->rotateChild(axis, Transforms.positions[id], Transforms.rotations[id], radians);
	Transforms.rotations[id] = normalize(Transforms.rotations[id]);
	// m.unlock();
}
void transform2::rotateChild(glm::vec3 axis, glm::vec3 pos, glm::quat r, float angle)
{
	// m.lock();
	glm::vec3 ax = r * axis;
	Transforms.updates[id].rot = true;
	Transforms.updates[id].pos = true;
	Transforms.positions[id] = pos + glm::rotate(Transforms.positions[id] - pos, angle, ax);
	Transforms.rotations[id] = glm::rotate(Transforms.rotations[id], angle, glm::inverse(Transforms.rotations[id]) * ax); // glm::rotate(rotation, angle, axis);
	for (transform2 a : Transforms.meta[id].children)
		a->rotateChild(axis, pos, r, angle);
	Transforms.rotations[id] = normalize(Transforms.rotations[id]);
	// m.unlock();
}

void transform2::orphan()
{
	if (Transforms.meta[id].parent.id == -1)
		return;
	Transforms.meta[getParent().id].m.lock();
	getParent().getChildren().erase(Transforms.meta[id].childId);
	Transforms.meta[getParent().id].m.unlock();
	Transforms.meta[id].parent.id = -1;
}

// bool compareTransform(transform2* t1, transform2* t2){
// 	return t1->_T < t2->_T;
// }

void initTransform()
{
	GPU_TRANSFORMS = new gpu_vector_proxy<_transform>();
	GPU_TRANSFORMS->usage = GL_STREAM_COPY;
	// GPU_TRANSFORMS_UPDATES = new gpu_vector_proxy<_transform>();
	// GPU_TRANSFORMS_UPDATES->usage = GL_STREAM_COPY;
	gpu_position_updates = new gpu_vector_proxy<glm::vec3>();
	gpu_position_updates->usage = GL_STREAM_COPY;
	gpu_rotation_updates = new gpu_vector_proxy<glm::quat>();
	gpu_rotation_updates->usage = GL_STREAM_COPY;
	gpu_scale_updates = new gpu_vector_proxy<glm::vec3>();
	gpu_scale_updates->usage = GL_STREAM_COPY;

	transformIds = new gpu_vector_proxy<GLint>();
	transformIds->usage = GL_STREAM_COPY;
}

void renderEdit(const char* name, transform2& t){
	if(t.id == -1) // uninitialized
		ImGui::InputText(name, "", 1, ImGuiInputTextFlags_ReadOnly);
	else if(t.name() == ""){
		string n = "game object " + to_string(t.id);
		ImGui::InputText(name, (char*)n.c_str(), n.size() + 1, ImGuiInputTextFlags_ReadOnly);
	}
	else 
		ImGui::InputText(name, (char *)t.name().c_str(), t.name().size() + 1, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TRANSFORM_DRAG_AND_DROP"))
		{
			IM_ASSERT(payload->DataSize == sizeof(int));
			int payload_n = *(const int *)payload->Data;
			t.id = payload_n;
		}
		ImGui::EndDragDropTarget();
	}
}
void saveTransforms(OARCHIVE &oa)
{
	oa << Transforms;
	// ofstream f("transform.lvl", std::fstream::binary);
	// _transform t;
	// for (int i = 0; i < Transforms.size(); i++)
	// {
	// 	t = transform2(i).getTransform();
	// 	t.parent = transform2(i).getParent().id;
	// 	f.write((char *)&t, sizeof(_transform));
	// }
	// f.close();
}

void setRootGameObject(transform2 r);


void loadTransforms(IARCHIVE &ia)
{
// 	ifstream f("transform.lvl", std::fstream::binary);
// 	f.seekg(0, f.end);
// 	size_t size = f.tellg();
// 	size /= sizeof(_transform);
// 	f.seekg(0, f.beg);
	// delete root2.gameObject();
	// root2 = transform2(0);
	// rootGameObject = new game_object(root2);
	Transforms.clear();
	ia >> Transforms;
	// setRootGameObject(root2);

	// for (int i = 0; i < size; i++)
	// {
	// 	Transforms._new();
	// }
	// ////////////////////////////////////
	// _transform _t;
	// f.read((char *)&_t, sizeof(_transform));
	// Transforms.positions[0] = _t.position;
	// Transforms.rotations[0] = _t.rotation;
	// Transforms.scales[0] = _t.scale;
	// Transforms.meta[0].parent = transform2(_t.parent);
	////////////////////////////////////
	for (int i = 1; i < Transforms.size(); i++)
	{
		// f.read((char *)&_t, sizeof(_transform));
		transform2 t(i);
		// // transform2 t2 = Transforms._new();
		// Transforms.positions[i] = _t.position;
		// Transforms.rotations[i] = _t.rotation;
		// Transforms.scales[i] = _t.scale;
		// Transforms.meta[i].parent = t.getParent();
		if(t.getParent().id != -1){
			Transforms.meta[t.getParent().id].children.push_back(t);
			Transforms.meta[i].childId = (--Transforms.meta[t.getParent().id].children.end());
			newGameObject(t);
		}
		// Transforms.meta[i].gameObject = new game_object()
	}
}

int switchAH(int index)
{
	return ~index - 1;
}
unsigned int transforms_enabled = 0;

Transform *root;

void Transform::init()
{
	// gameLock.lock();
	_T = TRANSFORMS._new();
	// gameLock.unlock();
	parent = 0;
	root->adopt(this);
}
game_object *Transform::gameObject()
{
	return this->_gameObject;
}
void Transform::setGameObject(game_object *g)
{
	this->_gameObject = g;
}
Transform::Transform(game_object *g) : m()
{
	this->setGameObject(g);
	init();
}

Transform::Transform(Transform &other, game_object *go) : m()
{
	this->setGameObject(go);
	init();
	_T->position = other.getPosition();
	_T->rotation = other.getRotation();
	_T->scale = other.getScale();
}

// Transform Transform::operator=(const Transform& t) {
// 	this->_T = t._T;
// 	this->gameObject = t.gameObject;
// 	this->children = t.children;
// 	this->parent = t.parent;
// 	return *this;
// }

void Transform::lookat(glm::vec3 direction, glm::vec3 up)
{
	_T->rotation = quatLookAtLH(direction, up);
}
glm::vec3 Transform::forward()
{
	return glm::normalize(_T->rotation * glm::vec3(0.0f, 0.0f, 1.0f));
}
glm::vec3 Transform::right()
{
	return glm::normalize(_T->rotation * glm::vec3(1.0f, 0.0f, 0.0f));
}
glm::vec3 Transform::up()
{
	return glm::normalize(_T->rotation * glm::vec3(0.0f, 1.0f, 0.0f));
}
glm::mat4 Transform::getModel()
{
	return (glm::translate(_T->position) * glm::toMat4(_T->rotation) * glm::scale(_T->scale));
}


glm::vec3 Transform::getScale()
{
	return _T->scale;
}
void Transform::setScale(glm::vec3 scale)
{
	this->scale(scale / _T->scale);
}
glm::vec3 Transform::getPosition()
{
	return _T->position;
}
void Transform::setPosition(glm::vec3 pos)
{
	// m.lock();
	for (Transform *c : children)
		c->translate(pos - _T->position, glm::quat());
	_T->position = pos;
	// m.unlock();
}
glm::quat Transform::getRotation()
{
	return _T->rotation;
}

void Transform::setRotation(glm::quat r)
{
	_T->rotation = r;
}
list<Transform *> &Transform::getChildren()
{
	return children;
}
Transform *Transform::getParent()
{
	return parent;
}
mutex m;
void Transform::adopt(Transform *transform)
{
	if (transform->parent == this)
		return;
	transform->orphan();
	transform->parent = this;
	this->m.lock();
	this->children.push_back(transform);
	transform->childId = (--this->children.end());
	this->m.unlock();
}

void Transform::_destroy()
{
	orphan();
	_T._delete();
	// if (enabled) {
	// 	TRANSFORMS._delete(_T);
	// }
	// else
	// {
	// 	STATIC_TRANSFORMS._delete(_T);
	// }
	// if (enabled)
	// 	--transforms_enabled;
	delete this;
}

void Transform::move(glm::vec3 movement, bool hasChildren)
{
	_T->position += movement;
	if (hasChildren)
	{
		for (auto a : children)
			a->translate(movement, glm::quat(1, 0, 0, 0));
	}
}
void Transform::translate(glm::vec3 translation)
{
	// m.lock();
	_T->position += _T->rotation * translation;
	for (auto a : children)
		a->translate(translation, _T->rotation);
	// m.unlock();
}
void Transform::translate(glm::vec3 translation, glm::quat r)
{
	// m.lock();
	_T->position += r * translation;
	for (Transform *a : children)
		a->translate(translation, r);
	// m.unlock();
}
void Transform::scale(glm::vec3 scale)
{
	// m.lock();
	_T->scale *= scale;
	for (Transform *a : children)
		a->scaleChild(_T->position, scale);
	// m.unlock();
}
void Transform::scaleChild(glm::vec3 pos, glm::vec3 scale)
{
	// m.lock();
	_T->position = (_T->position - pos) * scale + pos;
	_T->scale *= scale;
	for (Transform *a : children)
		a->scaleChild(pos, scale);
	// m.unlock();
}
void Transform::rotate(glm::vec3 axis, float radians)
{
	// m.lock();
	_T->rotation = glm::rotate(_T->rotation, radians, axis);
	for (Transform *a : children)
		a->rotateChild(axis, _T->position, _T->rotation, radians);
	_T->rotation = normalize(_T->rotation);
	// m.unlock();
}
void Transform::rotateChild(glm::vec3 axis, glm::vec3 pos, glm::quat r, float angle)
{
	// m.lock();
	glm::vec3 ax = r * axis;
	_T->position = pos + glm::rotate(_T->position - pos, angle, ax);
	_T->rotation = glm::rotate(_T->rotation, angle, glm::inverse(_T->rotation) * ax); // glm::rotate(rotation, angle, axis);
	for (Transform *a : children)
		a->rotateChild(axis, pos, r, angle);
	_T->rotation = normalize(_T->rotation);
	// m.unlock();
}

Transform::~Transform() {}
Transform::Transform(Transform &other)
{
}

void Transform::orphan()
{
	if (this->parent == 0)
		return;
	this->parent->m.lock();
	this->parent->children.erase(childId);
	this->parent->m.unlock();
	this->parent = 0;
}

bool compareTransform(Transform *t1, Transform *t2)
{
	return t1->_T < t2->_T;
}
