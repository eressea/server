#pragma once
#ifndef ERESSEA_EXPORT_H
#define ERESSEA_EXPORT_H

#define EXPORT_REGIONS  1<<0
#define EXPORT_FACTIONS 1<<1
#define EXPORT_UNITS    1<<2

struct stream;
int export_json(struct stream * out, unsigned int flags);

#endif
