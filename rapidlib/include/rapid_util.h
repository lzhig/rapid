#pragma once

#include "types_def.h"
#include <vector>
#include <stdlib.h>

namespace rapidlib
{
	void r_sleep(r_float64 sec);
	void r_sleep(r_uint32 ms);
	r_uint32 get_cpus();


#if (RAPID_PLATFORM == RAPID_PLATFORM_WIN32)
#define RAPID_THREAD_DECLARATION(func)       unsigned __stdcall func(void* args)
#else
#define RAPID_THREAD_DECLARATION(func)       void* func(void* args)
#endif

#if (RAPID_PLATFORM == RAPID_PLATFORM_WIN32)
r_int32 create_thread(unsigned __stdcall funcaddr(void*), void* pArgs);
#else
r_int32 create_thread(void* funcaddr(void*), void* pArgs);
#endif


class r_random_number
{
public:
	static void set_seed(r_uint32 t1, r_uint32 t2, r_uint32 t3)
	{
		srand(t1*t1 / 2);
		auto r1 = rand();
		srand(t2);
		auto r2 = rand();
		srand(t3);
		auto r3 = rand();

		srand(t1 + t2 + t3 + r1 + r2 + r3);
		auto c = rand() % 3;

		if (c == 0)
			srand(t1 + t2);
		else if (c == 1)
			srand(t2 + t3);
		else
			srand(t1 + t3);
	}
	static r_uint32 get_rand() { return rand(); }

	template<class T>
	static r_bool random_pick(std::vector<T>& src, r_uint32 num, std::vector<T>& desc)
	{
		auto size = src.size();
		if (size < num)
			return false;

		for (int i = 0; i < num; i++)
		{
			auto ndx = r_random_number::get_rand() % (size - i);

			desc.push_back(src[ndx]);
			if (ndx != size - 1)
			{
				std::swap(src[ndx], src[size - 1 - i]);
			}
		}

		return true;
	}

	template<class T>
	static r_bool random_pick(std::vector<T>& src, r_uint32 begin, r_uint32 range, r_uint32 num, std::vector<T>& desc)
	{
		auto size = src.size();
		if (size < begin + range || range < num)
			return false;

		for (int i = 0; i < num; i++)
		{
			auto ndx = begin + r_random_number::get_rand() % (range - i);

			desc.push_back(src[ndx]);
			if (ndx != range - 1)
			{
				std::swap(src[ndx], src[range - 1 - i]);
			}
		}

		return true;
	}

	template<class T>
	static void random_order(std::vector<T>& vec, r_uint32 num)
	{
		auto size = vec.size();
		for (int i = 0; i < num; i++)
		{
			for (int j = 0; j < size; j++)
			{
				auto ndx = r_random_number::get_rand() % size;

				if (ndx != size - 1)
				{
					std::swap(vec[ndx], vec[size - 1 - j]);
				}
			}
		}
	}
};
}