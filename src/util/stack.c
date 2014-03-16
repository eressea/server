#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void print_stack_trace(void)
{
    void* pointers[48];
    int backtrace_size =
        backtrace(pointers, sizeof(pointers) / sizeof(pointers[0]));
    char** symbols = backtrace_symbols(pointers, backtrace_size);
    int i;

    fputs("\n==== STACK BACKTRACE ====\n", stderr);
    for (i = backtrace_size - 1; i > 0; --i) {
        fprintf(stderr, "%3d: 0x%8.8x", i, (uintptr_t)pointers[i]);
        if (symbols) {
            fputc(' ', stderr);
            fputs(symbols[i], stderr);
        }
        fputc('\n', stderr);
    }
    fputs("=====================\n", stderr);
    if (symbols) free(symbols);
}
