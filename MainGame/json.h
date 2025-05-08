#pragma once
#include <vector>
#include <map>
#include <string>

struct json
{
	virtual int get_type() = 0;
	virtual void print(int indent = 0) = 0;
	virtual json* operator[](std::string x) { return nullptr; };
	struct json_object& obj() {
		return *(json_object *)this;
	}
	struct json_array& arr() {
		return *(json_array*)this;
	}
	struct json_string& str() {
		return *(json_string*)this;
	}
	struct json_number& num() {
		return *(json_number*)this;
	}
	void _indent(int i)
	{
		const char* q = "                                                                    ";
		printf("%.*s", i, q);
	}
};

struct json_object :json
{
	virtual int get_type();
	std::map<std::string, json*> content;
	~json_object();
	static json_object* read(const char*& ptr);
	virtual json* operator[](std::string x) {
		return content[x];
	}
	virtual void print(int indent = 0)
	{
		_indent(indent);
		printf("{\n");
		for (std::pair<const std::string, json*> x : content)
		{
			_indent(indent);
			printf("%s : ", x.first.c_str());
			x.second->print(indent + 1);
		}
		_indent(indent);
		printf("}\n");
	}
};
struct json_array :json
{
	virtual int get_type();
	std::vector<json*> content;
	~json_array();
	static json_array* read(const char*& ptr);
	virtual void print(int indent = 0)
	{
		_indent(indent);

		printf("[\n");
		for (json* x : content)
		{
			_indent(indent);
			x->print(indent + 1);
		}
		_indent(indent);
		printf("]\n");
	}
};
struct json_string : json
{
	virtual int get_type();
	std::string content;
	static json_string* read(const char*& ptr);
	virtual void print(int indent = 0)
	{
		_indent(indent);
		printf("\"%s\"\n", content.c_str());
	}
};
struct json_null :json
{
	virtual int get_type();
	static json_null* read(const char*& ptr);
	virtual void print(int indent = 0)
	{
		_indent(indent);
		printf("null\n");
	}
};
struct json_bool :json
{
	json_bool(bool c);
	virtual int get_type();
	bool content;
	static json_bool* read(const char*& ptr);
	virtual void print(int indent = 0)
	{
		_indent(indent);
		printf("%s\n", content?"true":"false");
	}
};
struct json_number :json
{
	json_number(float x);
	virtual int get_type();
	float content;
	static json_number* read(const char*& ptr);
	virtual void print(int indent = 0)
	{
		_indent(indent);
		printf("%f\n", content);
	}

};

json* parse_json(const char*& ptr);