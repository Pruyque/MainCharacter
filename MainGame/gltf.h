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
		while (p < (uint64_t)ptr + header->length)
		{
			printf("%.4s\n", &chunk->type);

			if (chunk->type == *(uint32_t*)"JSON")
			{
				const char* js = (const char*)(chunk + 1);
				json_object *obj = (json_object *)parse_json(js);

				printf("...\n");
			}
			if (chunk->type == *(uint32_t*)"BIN ")
			{

			}

			p += chunk->length + 8;
		}
	}

};

