#include <sched.h>
#ifdef NO_PTHEREAD_YIELD
int pthread_yield(void) {
	return sched_yield();
}
#endif
