#include "framework_impl.h"
#include "rapid.h"
#include "rapidjson/document.h"
#include <fstream>
#include <iosfwd>
#include <string>
#include "lib_manager.h"
#include "rapid_util.h"

#ifndef	__WINDOWS__
#include <sys/time.h>
#include "framework_impl.h"
#endif //__WINDOWS__

namespace rapidlib
{

	framework_impl::framework_impl()
		: m_lib_manager(this)
	{

	}

	framework_impl::~framework_impl()
	{
		m_lib_manager.shutdown();
	}

	void framework_impl::run()
	{
		auto t1 = m_time.now();
		auto t2 = t1;
		while (m_running)
		{
			t2 = m_time.now();
			const r_uint32 t = (t2 - t1) * 1000;
			if (t < 1)
			{
				r_sleep(0.01);
			}
			else
			{
				m_lib_manager.update(t);
				t1 = t2;
			}
		}
	}


#ifdef __WINDOWS__
	static DWORD_PTR	sMask = 0x00;
#endif //__WINDOWS__
	r_double64	framework_impl::time_util::m_tick = -1.0;

	framework_impl::time_util::time_util()
	{
#ifdef __WINDOWS__
		if (sMask != 0x00) return;
		const HANDLE	th = ::GetCurrentThread();

		const DWORD_PTR	m0 = ::SetThreadAffinityMask(th, 0x01);
		for (sMask = 0x80000000; sMask != 0x01; sMask >>= 1)
		{
			if ((sMask & m0) != 0x00)
			{
				// Set affinity to the first core
				::SetThreadAffinityMask(th, sMask);	break;
			}
		}

		LARGE_INTEGER freq;
		::QueryPerformanceFrequency(&freq);
		m_tick = 1.0 / freq.QuadPart;

		// Reset affinity
		::SetThreadAffinityMask(th, m0);
#else
		if (m_tick > 0.0) return;
		const time_t t = time(0);
		struct tm &tm = *localtime(&t);
		m_tick = t - (tm.tm_hour * 60 + tm.tm_min) * 60 - tm.tm_sec;
#endif //__WINDOWS__
	}

	r_double64 framework_impl::time_util::now()
	{
#ifdef __WINDOWS__
		// Set affinity to the first core
		const HANDLE	th = ::GetCurrentThread();
		const DWORD_PTR	m1 = ::SetThreadAffinityMask(th, sMask);

		LARGE_INTEGER time;
		::QueryPerformanceCounter(&time);

		// Reset affinity
		const DWORD_PTR	m2 = ::SetThreadAffinityMask(th, m1);

		return time.QuadPart * m_tick;
#else //__WINDOWS__
		struct timeval tv = { 0,0 };
		gettimeofday(&tv, 0);
		return (tv.tv_sec - m_tick) * 1.0 + tv.tv_usec * 0.000001;
#endif //__WINDOWS__
	}

}
