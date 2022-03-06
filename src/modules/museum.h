#pragma once
#ifndef HEADER_MUSEUM_H
#define HEADER_MUSEUM_H

extern struct attrib_type at_warden;
extern struct attrib_type at_museumexit;
extern struct attrib_type at_museumgivebackcookie;
extern struct attrib_type at_museumgiveback;

typedef struct {
    int warden_no;
    int cookie;
} museumgivebackcookie;

typedef struct {
    int cookie;
    struct item *items;
} museumgiveback;

void register_museum(void);

#endif
