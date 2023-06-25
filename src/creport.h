#pragma once
#ifndef H_GC_CREPORT
#define H_GC_CREPORT

#include <stdbool.h>

struct stream;
struct unit;
struct region;
struct faction;
enum seen_mode;

void creport_cleanup(void);
void register_cr(void);

int crwritemap(const char *filename);
void cr_output_region(struct stream* out, const struct faction* f,
    struct region* r, enum seen_mode mode);
void cr_output_unit(struct stream *out, const struct faction *f,
    const struct unit * u, enum seen_mode mode);
void cr_output_resources(struct stream *out, const struct faction *f,
    const struct region *r, enum seen_mode mode);
#endif
