#ifndef _TIME_SPEC_H_
#define _TIME_SPEC_H_
/* Windows lacks the nanosleep() function. The following code was stuffed
   together from GNUlib (http://www.gnu.org/software/gnulib/), which is
   licensed under the GPLv3. */
#include <time.h>
#include <errno.h>

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
/* Windows platforms.  */

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

int
nanosleep (const struct timespec *requested_delay,
           struct timespec *remaining_delay);

#endif
#endif
