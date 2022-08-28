#pragma once

#include <stdio.h>
#include <stdbool.h>

#include <util/order_parser.h>

struct unit;
struct faction;
struct order;

typedef struct parser_state {
    struct unit* u;
    struct faction* f;
    struct order** next_order;
} parser_state;

typedef struct input {
    const char *(*getbuf)(void *data);
    void *data;
} input;

OP_Parser parser_create(parser_state* state);
int parser_parse(OP_Parser parser, const char* input, size_t len, bool done);
void parser_free(OP_Parser parser);

void parser_set_unit(parser_state *state, struct unit *u);
void parser_set_faction(parser_state *state, struct faction *f);

int parseorders(FILE* F);
