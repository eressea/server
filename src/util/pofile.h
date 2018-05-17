#pragma once

#ifndef H_POFILE
#define H_POFILE

int pofile_read(const char *filename, int (*callback)(const char *msgid, const char *msgstr, const char *msgctxt, void *data), void *data);

#endif
