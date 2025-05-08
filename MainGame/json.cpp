#include "json.h"

void skip_white(const char*& ptr)
{
	while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')
		ptr++;
}

int json_object::get_type() {
	return 0;
}

json_object::~json_object()
{
	for (std::pair<const std::string, json*>& x : content)
		delete x.second;
}

json_object* json_object::read(const char*& ptr)
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
		ptr++;
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

int json_array::get_type() {
	return 1;
}

json_array::~json_array()
{
	for (json* x : content)
		delete x;
}

json_array* json_array::read(const char*& ptr)
{
	if (*ptr != '[')
		return 0;
	json_array* obj = new json_array;
	ptr++;
	skip_white(ptr);
	if (*ptr == ']')
	{
		ptr++;
		return obj;
	}
	while (1)
	{
		if (json* value = parse_json(ptr))
		{
			obj->content.push_back(value);
		}
		else
		{
			delete obj;
			return 0;
		}
		skip_white(ptr);
		if (*ptr == ']')
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

	return 0;
}

int json_string::get_type() {
	return 2;
}

json_string* json_string::read(const char*& ptr)
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

int json_null::get_type() {
	return 3;
}

json_null* json_null::read(const char*& ptr)
{
	if (*ptr != 'n')
		return 0;
	ptr += strlen("null");

	return new json_null;
}

json_bool::json_bool(bool c) :content(c)
{}

inline int json_bool::get_type() {
	return 4;
}

json_bool* json_bool::read(const char*& ptr)
{
	if (*ptr == 't')
	{
		ptr += strlen("true");
		return new json_bool(true);
	}
	if (*ptr == 'f')
	{
		ptr += strlen("false");
		return new json_bool(false);
	}
	return 0;
}

json_number::json_number(float x) :content(x) {}

int json_number::get_type() {
	return 5;
}

json_number* json_number::read(const char*& ptr)
{
	if (*ptr == '-' || *ptr == '.' || (*ptr >= '0' && *ptr <= '9'))
	{
		bool negative = false;
		if (*ptr == '-')
		{
			negative = true;
			ptr++;
		}
		float whole = 0;
		while (*ptr >= '0' && *ptr <= '9')
		{
			whole *= 10;
			whole += *ptr - '0';
			ptr++;
		}
		if (*ptr == '.')
		{
			ptr++;
			float mul = 0.1f;
			while (*ptr >= '0' && *ptr <= '9')
			{
				whole += (*ptr - '0') * mul;
				mul /= 10;
				ptr++;
			}
		}
		if (negative)
			whole = -whole;
		return new json_number(whole);
	}
	return 0;
}

json* parse_json(const char*& ptr)
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
