#pragma once
#include <openssl/md5.h>
#include "Public.h"

class Crypto
{
public:
	static Buffer MD5(const Buffer& text)
	{
		Buffer ret;
		Buffer data(16);
		MD5_CTX md5;
		MD5_Init(&md5);
		MD5_Update(&md5, text, text.size());
		MD5_Final((unsigned char*)data.c_str(), &md5);
		char tmp[3] = "";
		for (size_t i = 0; i < data.size(); i++)
		{
			snprintf(tmp, sizeof(tmp), "%02X", data[i] & 0xFF);
			ret += tmp;
		}
		return ret;
	}
};

