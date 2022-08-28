#ifdef HAVE_BACKTRACE
#include <signal.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
// IWYU pragma: no_include <bits/types/siginfo_t.h>
static void *btrace[50];

static void report_segfault(int signo, siginfo_t * sinf, void *arg)
{
    size_t size;
    int fd = fileno(stderr);

    fflush(stdout);
    fputs("\n\nProgram received SIGSEGV, backtrace follows.\n", stderr);
    size = backtrace(btrace, 50);
    backtrace_symbols_fd(btrace, size, fd);
    abort();
}

int setup_signal_handler(void)
{
    struct sigaction act;

    act.sa_flags = SA_RESETHAND | SA_SIGINFO;
    act.sa_sigaction = report_segfault;
    sigfillset(&act.sa_mask);
    return sigaction(SIGSEGV, &act, 0);
}
#else
int setup_signal_handler(void)
{
    return 0;
}
#endif

