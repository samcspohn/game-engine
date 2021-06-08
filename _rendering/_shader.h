#pragma once
#include <string>
#include <map>
#include "serialize.h"
#include "_rendering/Shader.h"
#include "editor.h"

using namespace std;
//shader data
struct _shader;
struct _shaderMeta : public assets::asset
{
	_shaderMeta();
	_shaderMeta(string compute);
	_shaderMeta(string vertex, string fragment);
	_shaderMeta(string vertex, string geom, string fragment);
	_shaderMeta(string vertex, string tess, string geom, string fragment);
	~_shaderMeta();
	bool onEdit();
	void inspect();
	string type();
	unique_ptr<Shader> shader = 0;
	friend struct _shader;
	SER_HELPER()
	{
		SER_BASE_ASSET
		ar &shader;
	}
};

namespace shaderManager
{

	extern map<size_t, shared_ptr<_shaderMeta>> shaders;
	extern map<int, shared_ptr<_shaderMeta>> shaders_ids;
	void _new();
	void destroy();
	void save(OARCHIVE &oa);
	void load(IARCHIVE &ia);
	void init();
}; // namespace shaderManager

class _shader
{
public:
	_shader();
	_shader(string compute);
	_shader(string vertex, string fragment);
	_shader(string vertex, string geom, string fragment);
	_shader(string vertex, string tess, string geom, string fragment);
	// _shaderMeta* s = 0;
	int s = 0;
	Shader &ref();
	Shader *operator->();
	_shaderMeta *meta() const;
	SER_HELPER()
	{
		ar &s;
	}
};

bool operator<(const _shader &l, const _shader &r);

namespace YAML
{
	template <>
	struct convert<_shader>
	{
		static Node encode(const _shader &rhs)
		{
			Node node;
			node["id"] = rhs.s;
			return node;
		}

		static bool decode(const Node &node, _shader &rhs)
		{
			rhs.s = node["id"].as<int>();
			return true;
		}
	};
}