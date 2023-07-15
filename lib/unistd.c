#include "../include/unistd.h"

_syscall0(pid_t, get_pid);
_syscall0(pid_t, get_ppid);
_syscall0(pid_t, fork);
_syscall2(int32_t, execv, const char *, path, const char **, argv);