#pragma once

#ifndef H_RENUMBER
#define H_RENUMBER

	struct unit;
	struct order;

    void renumber_factions(void);
    int renumber_cmd(struct unit *u, struct order *ord);

#endif
