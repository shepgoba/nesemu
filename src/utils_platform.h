#ifndef UTILS_PLATFORM_INCLUDE
#define UTILS_PLATFORM_INCLUDE
#include <time.h>
typedef struct {
	time_t time;
	uint64_t nanoseconds;
} precise_time_t;

precise_time_t get_precise_time();

#endif // UTILS_PLATFORM_INCLUDE
