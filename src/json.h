#pragma once
#ifndef ERESSEA_JSON_H
#define ERESSEA_JSON_H

#define EXPORT_REGIONS  1<<0
#define EXPORT_FACTIONS 1<<1
#define EXPORT_UNITS    1<<2

struct stream;
int json_export(struct stream * out, int flags);
int json_import(struct stream * out);

#endif
