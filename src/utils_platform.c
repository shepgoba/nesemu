#include <stdint.h>
#include "utils_platform.h"

#ifdef NESEMU_WINDOWS
#include <Windows.h>
#include <stdint.h>

static precise_time_t get_precise_time_win32(void)
{
	FILETIME ft;
	GetSystemTimePreciseAsFileTime(&ft);

	uint64_t ticks = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	const uint64_t EPOCH_DIFF_TICKS = 116444736000000000ULL;
	ticks -= EPOCH_DIFF_TICKS;

	precise_time_t pt;
	pt.time = (time_t)(ticks / 10000000ULL);
	pt.nanoseconds = (long)((ticks % 10000000ULL) * 100);

	return pt;
}
#elif defined NESEMU_MACOS || defined NESEMU_LINUX
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
#ifdef NESEMU_WINDOWS
	return get_precise_time_win32();
#elif defined NESEMU_MACOS || defined NESEMU_LINUX
	return get_precise_time_posix();
#else
	#error Unknown platform for get_precise_time
#endif
}
