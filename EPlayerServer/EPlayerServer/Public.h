#pragma once
#include <string>
#include <string.h>

class Buffer :public std::string
{
public:
	Buffer() :std::string() {}
	Buffer(int size) :std::string() { resize(size); }
	Buffer(const std::string& str) :std::string(str) {}
	Buffer(const char* str) :std::string(str) {}
	Buffer(const char* str, size_t len) :std::string()
	{
		if (str != NULL)
		{
			resize(len);
			memcpy((void*)c_str(), str, len);
		}
	}
	Buffer(const char* begin, const char* end) :std::string()
	{
		if (begin != NULL && end != NULL)
		{
			long int len = end - begin;
			if (len > 0)
			{
				resize(len);
				memcpy((void*)c_str(), begin, len);
			}
		}
	}
public:
	operator char* () { return (char*)c_str(); }
	operator char* ()const { return (char*)c_str(); }
	operator const char* ()const { return c_str(); }
	operator const void* ()const { return c_str(); }
};