#pragma once
#include <stdint.h>
#include <Windows.h>
#include <stdio.h>

#include <map>
#include <vector>
#include <string>

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

	struct json
	{
		virtual int get_type() = 0;
	};

	struct json_object:json
	{
		virtual int get_type() {
			return 0;
		}
		std::map<std::string, json*> content;
		~json_object()
		{
			for (std::pair<const std::string, json*>& x : content)
				delete x.second;
		}
		static json_object* read(const char*& ptr)
		{
			if (*ptr != '{')
				return 0;
			json_object* obj = new json_object;
			ptr++;

			skip_white(ptr);

			if (*ptr == '}')
			{
				ptr++;
				return obj;
			}

			while (1)
			{
				json_string* string = json_string::read(ptr);
				if (!string)
				{
					delete obj;
					return 0;
				}
				std::string key = string->content;
				delete string;
				skip_white(ptr);
				if (*ptr != ':')
				{
					delete obj;
					return 0;
				}
				ptr ++;
				skip_white(ptr);
				if (json* value = parse_json(ptr))
				{
					obj->content[key] = value;
				}
				else
				{
					delete obj;
					return 0;
				}
				skip_white(ptr);
				if (*ptr == '}')
				{
					ptr++;
					return obj;
				}
				if (*ptr != ',')
				{
					delete obj;
					return 0;
				}
				ptr++;
				skip_white(ptr);
			}
			// obj = '{' values '}'
			// values = value ',' values | value
			
			delete obj;
			return 0;
		}
	};
	struct json_array :json
	{
		virtual int get_type() {
			return 1;
		}
		std::vector<json*> content;
		~json_array()
		{
			for (json* x : content)
				delete x;
		}
		static json_array* read(const char*& ptr)
		{
			if (*ptr != '[')
				return 0;

			return 0;
		}
	};
	struct json_string : json
	{
		virtual int get_type() {
			return 2;
		}
		std::string content;
		static json_string* read(const char*& ptr)
		{
			if (*ptr != '"')
				return 0;
			ptr++;
			const char* start = ptr;
			while (*ptr++ != '"');

			json_string* obj = new json_string;
			obj->content = std::string(start, ptr - start - 1);
			return obj;
		}
	};
	struct json_null :json
	{
		virtual int get_type() {
			return 3;
		}
		static json_null* read(const char*& ptr)
		{
			if (*ptr != 'n')
				return 0;

			return 0;
		}
	};
	struct json_bool :json
	{
		virtual int get_type() {
			return 4;
		}
		bool content;
		static json_bool* read(const char*& ptr)
		{
			if (*ptr != 't' && *ptr != 'f')
				return 0;

			return 0;
		}
	};
	struct json_number :json
	{
		virtual int get_type() {
			return 5;
		}
		float content;
		static json_number* read(const char*& ptr)
		{
			if (*ptr == '-' || *ptr == '.' || (*ptr >= '0' && *ptr <= '9'))
			{
			}
			return 0;
		}
	};
	static void skip_white(const char*& ptr)
	{
		while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')
			ptr++;
	}
	static json* parse_json(const char* &ptr)
	{
		skip_white(ptr);
		if (json* obj = json_object::read(ptr))
			return obj;
		if (json* obj = json_array::read(ptr))
			return obj;
		if (json* obj = json_string::read(ptr))
			return obj;
		if (json* obj = json_null::read(ptr))
			return obj;
		if (json* obj = json_bool::read(ptr))
			return obj;
		if (json* obj = json_number::read(ptr))
			return obj;
		return 0;
	}
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
				json *obj = parse_json(js);

				printf("...\n");
			}

			p += chunk->length + 8;
		}
	}

};

