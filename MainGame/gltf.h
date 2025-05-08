#pragma once
#include <stdint.h>
#include <Windows.h>
#include <stdio.h>

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


	gltf(const void* ptr)
	{
		const header_t* header = (const header_t*)ptr;
		const chunk_hdr_t* chunk = (const chunk_hdr_t*)(header + 1);
		uint64_t& p = (uint64_t&)chunk;

		const char* js = 0;

		const char* data = 0;
		while (p < (uint64_t)ptr + header->length)
		{
			printf("%.4s\n", &chunk->type);

			if (chunk->type == *(uint32_t*)"JSON")
			{
				
				js = (const char*)(chunk + 1);
			}
			if (chunk->type == *(uint32_t*)"BIN\0")
			{
				data = (char *)(p + 8);
				printf("Binary data\n");
			}

			p += chunk->length + 8;
		}
		json_object& obj = *(json_object*)parse_json(js);
		json_array& accessors = obj["accessors"]->arr();
		json_array& views = obj["bufferViews"]->arr();
		json_array& meshes = obj["meshes"]->arr();
		for (json* entry : meshes.content)
		{
			entry->print();
			json_array& prim = entry->obj()["primitives"]->arr();
			for (json* p : prim.content)
			{
				int position = p->obj()["attributes"]->obj()["POSITION"]->num().content;
				int normal = p->obj()["attributes"]->obj()["NORMAL"]->num().content;
				int indices = p->obj()["indices"]->num().content;

				accessors.content[position]->print();
				views.content[(int)(accessors.content[position]->obj()["bufferView"]->num().content)]->print();
				accessors.content[normal]->print();
				accessors.content[indices]->print();
			}

		}

		json_array& scenes = obj["scenes"]->arr();
		for (json* entry : scenes.content)
		{

		}

		delete& obj;
		printf("...\n");

	}

};

