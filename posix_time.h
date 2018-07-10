#ifndef _TIME_SPEC_H_
#define _TIME_SPEC_H_
/* Windows lacks the nanosleep() function. The following code was stuffed
   together from GNUlib (http://www.gnu.org/software/gnulib/), which is
   licensed under the GPLv3. */
#include <time.h>
#include <errno.h>
#include "Zaimoni.STL/Pure.C/comptest.h"	/* for C library test results */

#ifndef ZAIMONI_HAVE_POSIX_NANOSLEEP_IN_TIME_H
/* Above should exclude Sufficiently POSIX Systems, including Cygwin */

#ifndef ZAIMONI_HAVE_POSIX_TIMESPEC_IN_TIME_H
/* MingWin 5.4- and older MSVC's don't have this in time.h.  MSVC 2017 does.  */

/* however, pthread.h *does* provide this and is force-included on some flavors of MingW32 GCC 5.1 */
#if _TIMESPEC_DEFINED
#   ifdef __cplusplus
extern "C" {
#   endif

struct timespec
{
  time_t tv_sec;
  long int tv_nsec;
};

#   ifdef __cplusplus
}
#   endif
#endif
#endif

int nanosleep (const struct timespec *requested_delay, struct timespec *remaining_delay);

#endif
#endif
