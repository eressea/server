## Eliminate non-ascii characters from the code

In Linux, file *.c will output this:

    study.c:          C source, ISO-8859 text
    summary.c:        C source, ISO-8859 text
    test_eressea.c:   C source, ASCII text

Let's make all the source files ASCII. In a lot of cases, that means
rewriting German comments to be English, which is also great.

## remove platform.h, use rtl.h

In Atlantis, I use a header called rtl.h to polyfill functions like
_strdup and _snprintf. I would like that to be a reusable library that
Eressea can use, and eliminate the hacks in platform.h in favor of
autoconf-style detections done by CMake (already happening for a few
things).

