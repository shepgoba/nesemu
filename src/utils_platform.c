#include <stdint.h>
#include "utils_platform.h"

#ifdef _WIN32
#include <Windows.h>
static precise_time_t get_precise_time_win32(void)
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	uint64_t total_us = (((uint64_t)ft.dwHighDateTime << 32) | (uint64_t)ft.dwLowDateTime) / 10;
	const uint64_t EPOCH_DIFF_US = 11644473600ULL * 1000000ULL;

	total_us -= EPOCH_DIFF_US;

	precise_time_t pt;
	pt.time = (time_t)(total_us / 1000000ULL);
	pt.nanoseconds = (total_us % 1000000ULL) * 1000;

	return pt;
}
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
static precise_time_t get_precise_time_posix(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	precise_time_t pt;
	pt.time = ts.tv_sec;
	pt.nanoseconds = ts.tv_nsec;

	return pt;
}
#endif


precise_time_t get_precise_time()
{
#ifdef WIN32
	return get_precise_time_win32();
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
	return get_precise_time_posix();
#else
	#error Unknown platform for get_precise_time
#endif
}
