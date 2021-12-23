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
	shared_ptr<Shader> shader = 0;
	friend struct _shader;
};

class shaderManager : public assets::assetManager<_shaderMeta>
{
public:
	void _new();
	void destroy();
	// void encode(YAML::Node &node);
	// void decode(YAML::Node &node);
	void init();
}; // namespace shaderManager

extern shaderManager shader_manager;

class _shader  : public assets::asset_instance<_shaderMeta>
{
public:
	_shader();
	_shader(string compute);
	_shader(string vertex, string fragment);
	_shader(string vertex, string geom, string fragment);
	_shader(string vertex, string tess, string geom, string fragment);
	// _shaderMeta* s = 0;
	Shader &ref();
	Shader *operator->();
	_shaderMeta *meta() const;
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
			node["id"] = rhs.id;
			return node;
		}

		static bool decode(const Node &node, _shader &rhs)
		{
			rhs.id = node["id"].as<int>();
			return true;
		}
	};

	template<>
	struct convert<_shaderMeta>
	{
		static Node encode(const _shaderMeta &rhs)
		{
			Node node;
			// node["asset"] = *(assets::asset*)&rhs;
			node["id"] = rhs.id;
            node["name"] = rhs.name;
			if(rhs.shader)
			{
				node["shader"] = *rhs.shader;
			}else{
			}
			return node;
		}

		static bool decode(const Node &node, _shaderMeta &rhs)
		{
			rhs.id = node["id"].as<int>();
            rhs.name = node["name"].as<string>();
			rhs.shader = make_unique<Shader>(node["shader"].as<Shader>());
			return true;
		}
	};
}