#pragma once
#ifndef H_MOD_GMCMD
#define H_MOD_GMCMD

struct plane;
struct attrib;
struct unit;
struct faction;
struct region;

void register_gmcmd(void);
/* initialize this module */

void gmcommands(void);
/* execute commands */

struct faction *gm_addfaction(const char *email, struct plane *p,
struct region *r);
struct plane *gm_addplane(int radius, unsigned int flags,
    const char *name);

/*
    * doesn't belong in here:
    */
struct attrib *find_key(struct attrib *attribs, int key);

#endif
