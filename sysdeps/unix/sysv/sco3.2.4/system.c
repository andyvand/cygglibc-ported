/* SCO 3.2v4 does have `waitpid'.
   Avoid unix/system.c, which says we don't.  */
#include <sysdeps/posix/system.c>
