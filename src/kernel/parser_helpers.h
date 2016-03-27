/*
  parser_helpers.h

  Created on: Mar 14, 2016
  Author: steffen

Copyright (c) 2016, Enno Rehling <enno@eressea.de>
Steffen Mecke <stm2@users.sf.net>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/


#ifndef H_KRNL_PARSER_HELPER
#define H_KRNL_PARSER_HELPER
#ifdef __cplusplus
extern "C" {
#endif

struct building *getbuilding(const struct region *r);
struct ship *getship(const struct region *r);

#ifdef __cplusplus
}
#endif

#endif /* H_KRNL_PARSER_HELPER */
