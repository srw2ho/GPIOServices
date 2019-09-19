//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <collection.h>
#include <ppltasks.h>
#include <agile.h>

namespace BME280Service
{
	inline int64_t DeltaTime(const SYSTEMTIME st1, const SYSTEMTIME st2) {

		union timeunion { FILETIME fileTime; ULARGE_INTEGER ul; };
		timeunion ft1;
		timeunion ft2;
		SystemTimeToFileTime(&st1, &ft1.fileTime);
		SystemTimeToFileTime(&st2, &ft2.fileTime);

		return ft2.ul.QuadPart - ft1.ul.QuadPart;
	}

	inline UINT64 FileTimeToMillis(const FILETIME &ft)
	{
		ULARGE_INTEGER uli;
		uli.LowPart = ft.dwLowDateTime; // could use memcpy here!
		uli.HighPart = ft.dwHighDateTime;

		return static_cast<UINT64>(uli.QuadPart / 10000);
	}
	inline UINT64 getActualUnixTime()
	{
		SYSTEMTIME st;
		FILETIME ft;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);

		UINT64 UnixTime = FileTimeToMillis((const FILETIME&)ft);
		return UnixTime;
	}

	inline void MillisToSystemTime(UINT64 millis, SYSTEMTIME *st)
	{
		UINT64 multiplier = 10000;
		UINT64 t = multiplier * millis;

		ULARGE_INTEGER li;
		li.QuadPart = t;
		// NOTE, DON'T have to do this any longer because we're putting
		// in the 64bit UINT directly
		//li.LowPart = static_cast<DWORD>(t & 0xFFFFFFFF);
		//li.HighPart = static_cast<DWORD>(t >> 32);

		FILETIME ft;
		ft.dwLowDateTime = li.LowPart;
		ft.dwHighDateTime = li.HighPart;

		::FileTimeToSystemTime(&ft, st);
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