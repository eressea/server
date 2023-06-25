#pragma once
#ifndef H_ERESSEA_LIB
#define H_ERESSEA_LIB
struct locale;

void game_init(void);
void game_done(void);

void free_gamedata(void);
void free_configuration(void);
void init_locale(struct locale *lang);

#endif
