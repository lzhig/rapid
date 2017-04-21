#include "rapid_util.h"

#if RAPID_PLATFORM == RAPID_PLATFORM_WIN32
#include <windows.h>
#include <process.h>
#else
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#endif

#define MAX_CPU			(64)

namespace rapidlib
{
	
	void r_sleep(r_float64 sec)
	{
#if RAPID_PLATFORM == RAPID_PLATFORM_WIN32
		DWORD milli_sec = static_cast<DWORD>((sec + 0.0005) * 1000.0);
		::Sleep(milli_sec);
#   else
		struct timespec t;
#define N_NANOSEC_INT 1000000000
#define N_NANOSEC_FLT 1000000000.0
		long long int nano_sec = (long long int) (sec * N_NANOSEC_FLT);
		t.tv_sec = nano_sec / N_NANOSEC_INT;
		t.tv_nsec = nano_sec % N_NANOSEC_INT;
		nanosleep(&t, NULL);
#   endif
	}

	void r_sleep(r_uint32 ms)
	{
	}

	r_uint32 get_cpus()
	{
#if (RAPID_PLATFORM == RAPID_PLATFORM_WIN32)
		SYSTEM_INFO info = {};
		::GetSystemInfo(&info);
		return (info.dwNumberOfProcessors < MAX_CPU) ? info.dwNumberOfProcessors : MAX_CPU;
#else
		FILE* fp = fopen("/proc/cpuinfo", "rb");
		if (fp != NULL)
		{
			const r_uint32 sz = 1024 * MAX_CPU;
			char buff[sz] = {};
			fread(buff, 1, sz - 1, fp);
			fclose(fp);
			r_uint8 nNum = 0; const char* cpu = "processor";
			for (const char* str = strstr(buff, cpu); str != NULL; str = strstr(str + 9, cpu))
			{
				nNum++;
			}
			return (nNum < 1) ? 1 : (nNum < MAX_CPU ? nNum : MAX_CPU);
		}
		return 1;
#endif
	}

#if (RAPID_PLATFORM == RAPID_PLATFORM_WIN32)
	int create_thread(unsigned __stdcall funcaddr(void*), void* pArgs)
	{
		HANDLE hThreadHandle = NULL; unsigned nThreadID = 0;
		hThreadHandle = (HANDLE)_beginthreadex(NULL, 0x100000 * 2, funcaddr, pArgs, 0, &nThreadID);
		if (hThreadHandle == NULL)
		{
			return 1;
		}
		CloseHandle(hThreadHandle);
		return 0;
	}
#else
	int create_thread(void* funcaddr(void*), void* pArgs)
	{
		pthread_t hThreadHandle = 0;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		return pthread_create(&hThreadHandle, &attr, funcaddr, pArgs);
	}
#endif

}