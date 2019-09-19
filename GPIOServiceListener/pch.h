#pragma once

#include <collection.h>
#include <ppltasks.h>

namespace GPIOServiceListener
{

	inline std::vector<std::wstring> splitintoArray(const std::wstring& s, const std::wstring& delim)
	{
		std::vector<std::wstring> result;
		wchar_t*pbegin = (wchar_t*)s.c_str();
		wchar_t * next_token = nullptr;
		wchar_t* token;
		token = wcstok_s(pbegin, delim.c_str(), &next_token);
		while (token != nullptr) {
			// Do something with the tok
			result.push_back(token);
			token = wcstok_s(NULL, delim.c_str(), &next_token);
		}

		return result;
	}

	inline Platform::String^ StringFromAscIIChars(char* chars)
	{
		size_t newsize = strlen(chars) + 1;
		wchar_t * wcstring = new wchar_t[newsize];
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, wcstring, newsize, chars, _TRUNCATE);
		Platform::String^ str = ref new Platform::String(wcstring);
		delete[] wcstring;
		return str;
	}
}