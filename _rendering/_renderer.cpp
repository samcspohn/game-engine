#include "_rendering/_renderer.h"


_shader _quadShader;

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
void _renderer::onStart()
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
		renderingManager::shader_model_vector[s.s][m.m] = new renderingMeta(s, m);

		r = renderingManager::shader_model_vector.find(s.s);
		// updatedRenderMetas.push_back(r->second[m.m->name]);
	}
	else
	{
		auto rm = r->second.find(m.m);
		if (rm == r->second.end())
		{
			r->second[m.m] = new renderingMeta(s, m);
			// updatedRenderMetas.push_back(r->second[m.m->name]);
		}
	}
	renderingManager::unlock();

	meta = (r->second[m.m]);
		
	transformIdRef = meta->ids.push_back(transform.id);
}

void _renderer::set_proto(_shader s, _model m)
{
	shader = s;
	model = m;
	renderingManager::lock();
	auto r = renderingManager::shader_model_vector.find(s.s);
	if (r == renderingManager::shader_model_vector.end())
	{
		renderingManager::shader_model_vector[s.s][m.m] = new renderingMeta(s, m);

		r = renderingManager::shader_model_vector.find(s.s);
	}
	else
	{
		auto rm = r->second.find(m.m);
		if (rm == r->second.end())
			r->second[m.m] = new renderingMeta(s, m);
	}
	renderingManager::unlock();

	meta = (r->second[m.m]);
}

void _renderer::set(renderingMeta *_meta)
{
	shader = _meta->s;
	model = _meta->m;
	// if (modelManager::models[model.m]->unique)
	// {
	// 	model.makeUnique();
	// 	this->set(shader, model);
	// }
	// else
	// {
	meta = _meta;
	// if (!transformIdRef.isNull())
	// {
	// 	// meta->ids._delete(transformIdRef);
	// }
	// meta->ids.push_back(transform.id);
	// transformIdRef = --meta->ids.end();
	transformIdRef = meta->ids.push_back(transform.id);
	// }
}
_renderer::_renderer(const _renderer &other)
{
	shader = other.shader;
	model = other.model;
	meta = other.meta;
	// transformIdRef = fast_list_deque<GLuint>::iterator();
}
void _renderer::onDestroy()
{
	// transformIdRef._delete();
	if (meta != 0)
	{
		meta->ids.erase(transformIdRef);
	}
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
gpu_vector<__renderer> *__RENDERERS_in;
// gpu_vector_proxy<__renderer> *__RENDERERS_out;
gpu_vector<GLuint> *__renderer_offsets;
gpu_vector<__renderMeta> *__rendererMetas;