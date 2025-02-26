#pragma once
#ifndef H_KERNEL_GROUP
#define H_KERNEL_GROUP

typedef struct group {
    struct group *next;
    struct group *nexthash;
    struct faction *f;
    struct attrib *attribs;
    char *name;
    struct ally *allies;
    int gid;
    int members;
} group;

struct gamedata;
struct unit;
struct faction;

extern struct attrib_type at_group;   /* attribute for units assigned to a group */

struct group *join_group(struct unit *u, const char *name);
void set_group(struct unit *u, struct group *g);
struct group * get_group(const struct unit *u);
void free_group(struct group *g);
struct group *group_create(struct faction *f, int gid);
struct group *create_group(struct faction * f, const char *name, int gid);
struct group* find_groupbyname(struct faction* f, const char* name);

void write_groups(struct gamedata *data, const struct faction *f);
void read_groups(struct gamedata *data, struct faction *f);

#endif
