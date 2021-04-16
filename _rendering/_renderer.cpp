#include "_rendering/_renderer.h"
#define GLM_GTX_intersect
#include <glm/gtx/intersect.hpp>
#include "physics/collision.h"

_shader _quadShader;

__renderer::__renderer(){}
__renderer::__renderer(uint _transform, uint _id) : transform{_transform},id{_id} { }

void destroyRendering()
{
	shaderManager::destroy();
	modelManager::destroy();
	renderingManager::destroy();
}
_model _renderer::getModel()
{
	return model;
}
_shader _renderer::getShader()
{
	return shader;
}


void _renderer::setCullSizes(float min, float max)
{
	meta->maxRadius = max;
	meta->minRadius = min;
}
void _renderer::init(int id)
{
	if (shader.s == 0 || model.m == 0)
		return;
	if (meta != 0)
		set(meta);
	else
		set(shader, model);
}
_renderer::_renderer() {}

void _renderer::set(_shader s, _model m)
{
	shader = s;
	model = m;
	if(transformIdRef != -1)
		meta->ids._delete(transformIdRef);
	// if (!transformIdRef.isNull())
	// {
	// 	meta->ids.erase(transformIdRef);
	// }
	if (s.s == 0 || m.m == 0)
		return;
	renderingManager::lock();
	auto r = renderingManager::shader_model_vector.find(s.s);
	if (r == renderingManager::shader_model_vector.end())
	{
		renderingManager::shader_model_vector[s.s][m.m] = make_unique<renderingMeta>(s, m);

		r = renderingManager::shader_model_vector.find(s.s);
		// updatedRenderMetas.push_back(r->second[m.m->name]);
	}
	else
	{
		auto rm = r->second.find(m.m);
		if (rm == r->second.end())
		{
			r->second[m.m] = make_unique<renderingMeta>(s, m);
			// updatedRenderMetas.push_back(r->second[m.m->name]);
		}
	}
	renderingManager::unlock();

	meta = (r->second[m.m]).get();
		
	transformIdRef = meta->ids._new(static_cast<GLuint>(transform.id));
}

void _renderer::set_proto(_shader s, _model m)
{
	shader = s;
	model = m;
	renderingManager::lock();
	auto r = renderingManager::shader_model_vector.find(s.s);
	if (r == renderingManager::shader_model_vector.end())
	{
		renderingManager::shader_model_vector[s.s][m.m] = make_unique<renderingMeta>(s, m);

		r = renderingManager::shader_model_vector.find(s.s);
	}
	else
	{
		auto rm = r->second.find(m.m);
		if (rm == r->second.end())
			r->second[m.m] = make_unique<renderingMeta>(s, m);
	}
	renderingManager::unlock();

	meta = (r->second[m.m]).get();
}

void _renderer::set(renderingMeta *_meta)
{
	if(transformIdRef != -1)
		meta->ids._delete(transformIdRef);
	
	shader = _meta->s;
	model = _meta->m;
	meta = _meta;
	transformIdRef = meta->ids._new(transform.id);
}
_renderer::_renderer(const _renderer &other)
{
	shader = other.shader;
	model = other.model;
	meta = other.meta;
	transformIdRef = -1;//meta->ids._new(transform.id);
	// transformIdRef = fast_list_deque<GLuint>::iterator();
}
void _renderer::deinit(int id)
{
	if (meta != 0)
		meta->ids._delete(transformIdRef);
	// transformIdRef._delete();
	// if (meta != 0)
	// {
	// 	meta->ids.erase(transformIdRef);
	// }
}

void renderEdit(const char *name, _model &m)
{
	// ImGui::DragInt(name,&i);
	if (m.m == 0) // uninitialized
		ImGui::InputText(name, "", 1, ImGuiInputTextFlags_ReadOnly);
	else
		ImGui::InputText(name, (char *)m.meta()->name.c_str(), m.meta()->name.size() + 1, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("MODEL_DRAG_AND_DROP"))
		{
			IM_ASSERT(payload->DataSize == sizeof(int));
			int payload_n = *(const int *)payload->Data;
			m.m = payload_n;
		}
		ImGui::EndDragDropTarget();
	}
}

void renderEdit(const char *name, _shader &s)
{
	// ImGui::DragInt(name,&i);
	if (s.s == 0) // uninitialized
		ImGui::InputText(name, "", 1, ImGuiInputTextFlags_ReadOnly);
	else
		ImGui::InputText(name, (char *)s.meta()->name.c_str(), s.meta()->name.size() + 1, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SHADER_DRAG_AND_DROP"))
		{
			IM_ASSERT(payload->DataSize == sizeof(int));
			int payload_n = *(const int *)payload->Data;
			s.s = payload_n;
		}
		ImGui::EndDragDropTarget();
	}
}

void _renderer::onEdit()
{
	_shader curr_s = shader;
	_model curr_m = model;
	RENDER(model);
	RENDER(shader);

	if (model.m != curr_m.m || shader.s != curr_s.s)
	{
		this->set(shader, model);
	}
}

gpu_vector_proxy<matrix> *GPU_MATRIXES;
gpu_vector_proxy<__renderer> *__RENDERERS_in;
int __RENDERERS_in_size;
// gpu_vector_proxy<__renderer> *__RENDERERS_out;
gpu_vector<GLuint> *__renderer_offsets;
gpu_vector<__renderMeta> *__rendererMetas;


bool intersectRayMesh(glm::vec3 &p, glm::vec3 &d, Mesh *m, glm::vec3 &q, float &dist, glm::mat4 &model)
{

	float closest = numeric_limits<float>().max();
	glm::vec3 r{closest};
	bool ret = false;
	for (int i = 0; i < m->indices.size(); i += 3)
	{
		glm::vec3 p1 = model * glm::vec4(m->vertices[m->indices[i]], 1);
		glm::vec3 p2 = model * glm::vec4(m->vertices[m->indices[i + 1]], 1);
		glm::vec3 p3 = model * glm::vec4(m->vertices[m->indices[i + 2]], 1);
		// if (IntersectSegmentTriangle(p, p + d * closest, p1, p2, p3, q.x,q.y,q.z, t))
		// glm::vec2 bp;
		// float t;
		// if(glm::intersectRayTriangle(p,d,p1,p2,p3, bp, t))
		// if (glm::intersectLineTriangle(p, d, p1, p2, p3, q))
		if(_intersectRayTriangle(p,d,p1,p2,p3,q))
		{
			// float dist = t;
			dist = glm::length(p - q);
			if (dist < closest)
			{
				r = q;
				closest = dist;
			}
			ret = true;
		}
	}
	dist = closest;
	q = r;
	return ret;
}
bool intersectRayModel(glm::vec3 &p, glm::vec3 &d, Model *M, glm::vec3 &q, float &dist, glm::mat4 &model, glm::mat4 &rot)
{
	// glm::mat4 invModel = glm::inverse(model);
	// p = invModel * glm::vec4(p, 1);
	// d = glm::inverse(rot) * glm::vec4(d, 1);
	bool ret = false;
	float closest = numeric_limits<float>().max();
	glm::vec3 r{closest};
	for (Mesh *m : M->meshes)
	{
		if (intersectRayMesh(p, d, m, q, dist, model))
		{

			dist = glm::length(p - q);
			if (dist < closest)
			{
				r = q;
				closest = dist;
			}
			ret = true;
		}
	}
	dist = closest;
	q = r;
	return ret;
}

transform2 renderRaycast(glm::vec3 p, glm::vec3 dir)
{
	dir = normalize(dir);
	auto renderers = COMPONENT_LIST(_renderer);
	cout << "p: " + to_string(p) + " dir: " + to_string(dir) + '\n';
	// ray _ray(p, dir);

	float closest = numeric_limits<float>().max();
	transform2 ret;
	mutex lock;
	parallelfor(renderers->size(), 
	// for (uint32_t i = 0; i < renderers->size(); ++i)
	{
	
		glm::vec3 q;
		_renderer *_rend;
		if (renderers->getv(i) && (_rend = renderers->get(i))->getModel().meta())
		{
			// if (_rend->transform->gameObject()->getComponent<terrain>() != 0)
			// 	continue;
			// _renderer *_rend = renderers->get(i);
			float rad = _rend->getModel().meta()->radius;
			float sc = glm::length(_rend->transform->getScale());
			rad *= sc;
			// if (glm::intere(p,dir,sphere(_rend->transform->getPosition(),rad),t,q))
			float t;
			if (glm::intersectRaySphere(p, dir, _rend->transform->getPosition(), rad * rad, t))
			{
				string name = renderers->get(i)->transform->name();
				if (name == "")
				{
					name = "game object " + to_string(_rend->transform.id);
				}
				glm::mat4 model = _rend->transform->getModel();
				glm::mat4 rot = glm::toMat4(_rend->transform->getRotation());
				if (intersectRayModel(p, dir, _rend->getModel().meta()->model.get(), q, t, model, rot))
				{
					cout << name + ": hit -- " + to_string(t) + "\n";
					// float dist = t;
					float dist = glm::length(p - q);
					{
						scoped_lock l(lock);
						if (dist < closest)
						{
							closest = dist;
							ret = _rend->transform;
						}
					}
				}
				else
				{
					cout << name + ": miss\n";
				}
			}
		}
	})
	// }
	return ret;
}