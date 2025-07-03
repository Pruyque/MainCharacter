#pragma once
#include <stdint.h>
#include <Windows.h>
#include <stdio.h>

#include <gl/GL.h>

#include <map>
#include <vector>
#include <string>

#include "json.h"



struct gltf
{
	struct header_t
	{
		uint32_t sig;
		uint32_t version;
		uint32_t length;
	};

	struct chunk_hdr_t
	{
		uint32_t length;
		uint32_t type;
	};

	int size;
	char* buffer;

	struct accessor_t
	{
		int compontent_type;
		int count;
		int view;
		std::string type;
		accessor_t() = default;
		accessor_t(const accessor_t& other) = default;
		accessor_t(json& json)
		{

		}
	};

	struct primitive_t
	{
		int position, normal, indices, mode;
	};

	struct mesh_t
	{
		std::string name;
		std::vector<primitive_t> primitives;
	};

	struct view_t
	{
		int size, offset;
	};

	std::vector<view_t> views;
	std::vector<accessor_t> accessors;
	std::map<std::string, mesh_t> named_meshes;

	void draw(std::string name) try
	{
		mesh_t& mesh = named_meshes[name];
		for (primitive_t& prim : mesh.primitives)
		{
			view_t& indices_view = views[accessors[prim.indices].view];
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, accessors[prim.position].compontent_type, 0, buffer + views[accessors[prim.position].view].offset);
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(accessors[prim.normal].compontent_type, 0, buffer + views[accessors[prim.normal].view].offset);
			glDrawElements(GL_TRIANGLES, accessors[prim.indices].count, accessors[prim.indices].compontent_type, buffer + indices_view.offset);
		}
	}
	catch (...)
	{

	}

	~gltf()
	{
		delete buffer;
	}

	gltf(const void* ptr)
	{
		const header_t* header = (const header_t*)ptr;
		const chunk_hdr_t* chunk = (const chunk_hdr_t*)(header + 1);
		uint64_t& p = (uint64_t&)chunk;

		const char* js = 0;

		const char* data = 0;
		int data_size;
		while (p < (uint64_t)ptr + header->length)
		{
			if (chunk->type == *(uint32_t*)"JSON")
			{
				
				js = (const char*)(chunk + 1);
			}
			if (chunk->type == *(uint32_t*)"BIN\0")
			{
				data = (char *)(p + 8);
				data_size = chunk->length;
			}

			p += chunk->length + 8;
		}

		buffer = new char[data_size];
		memcpy(buffer, data, data_size);


		json_object& obj = *(json_object*)parse_json(js);
		json_array& accessors = obj["accessors"]->arr();
		json_array& views = obj["bufferViews"]->arr();
		json_array& meshes = obj["meshes"]->arr();
		for (json* entry : accessors.content)
		{
			accessor_t a;
			a.compontent_type = (int)entry->obj()["componentType"]->num().content;
			a.count = (int)entry->obj()["count"]->num().content;
			a.type = entry->obj()["type"]->str().content;
			a.view = (int)entry->obj()["bufferView"]->num().content;
			this->accessors.push_back(a);
		}
		for (json* entry : views.content)
		{
			view_t v;
			v.offset = entry->obj()["byteOffset"]->num().content;
			v.size = entry->obj()["byteLength"]->num().content;
			this->views.push_back(v);
		}
		for (json* entry : meshes.content)
		{
			mesh_t m;
			m.name = entry->obj()["name"]->str().content;

			json_array& prim = entry->obj()["primitives"]->arr();
			for (json* p : prim.content)
			{
				int position = (int)p->obj()["attributes"]->obj()["POSITION"]->num().content;
				int normal = (int)p->obj()["attributes"]->obj()["NORMAL"]->num().content;
				int indices = (int)p->obj()["indices"]->num().content;
				int mode = 4;

				if (p->obj().content.find("mode") != p->obj().content.end())
				{
					mode = p->obj().content["mode"]->num().content;
				}
				primitive_t p;
				p.position = position;
				p.normal = normal;
				p.indices = indices;
				p.mode = mode;
				m.primitives.push_back(p);
			}
			this->named_meshes[m.name] = m;
		}

		delete& obj;

	}

};

