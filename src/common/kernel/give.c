/* vi: set ts=2:
+-----------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
| Eressea II host |  Enno Rehling <enno@eressea.de>
| (c) 1998 - 2003 |  Katja Zedel <katze@felidae.kn-bremen.de>
+-----------------+  

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <config.h>
#include "eressea.h"

#include "give.h"

/* kernel includes */
#include "message.h"
#include "reports.h"
#include "unit.h"
#include "pool.h"
#include "faction.h"
#include "region.h"
#include "item.h"
#include "skill.h"

/* items includes */
#include <items/money.h>

/* attributes includes */
#include <attributes/racename.h>
#include <attributes/orcification.h>

/* util includes */
#include <util/event.h>

/* libc includes */
#include <limits.h>
#include <stdlib.h>

/* Wieviel Fremde eine Partei pro Woche aufnehmen kann */
#define MAXNEWBIES								5

static int 
GiveRestriction(void) {
	static int value = -1;
	if (value<0) {
		value = atoi(get_param(global.parameters, "GiveRestriction"));
	}
	return value;
}

static void
add_give(unit * u, unit * u2, int n, const resource_type * rtype, struct order * ord, int error)
{
	if (error) {
		cmistake(u, ord, error, MSG_COMMERCE);
	} else if (!u2 || ufaction(u2)!=ufaction(u)) {
		assert(rtype);
		add_message(&ufaction(u)->msgs,
			msg_message("give", "unit target resource amount",
			u, u2?ucansee(ufaction(u), u2, u_unknown()):u_peasants(), rtype, n));
		if (u2) {
			add_message(&ufaction(u2)->msgs,
				msg_message("give", "unit target resource amount",
				ucansee(ufaction(u2), u, u_unknown()), u2, rtype, n));
		}
	}
}

static void
addgive(unit * u, unit * u2, int n, resource_t res, struct order * ord, int error)
{
	add_give(u, u2, n, oldresourcetype[res], ord, error);
}

static void
give_peasants(int n, const item_type * itype, unit * src)
{
	region *r = uregion(src);

	/* horses are given to the region via itype->give! */

	if (itype->rtype==itype_money->rtype) {
		rsetmoney(r, rmoney(r) + n);
	}
}

int
give_item(int want, const item_type * itype, unit * src, unit * dest, struct order * ord)
{
	short error = 0;
	int n;

	assert(itype!=NULL);
	n = new_get_pooled(src, item2resource(itype), GET_DEFAULT);
	n = min(want, n);
	if (dest && src->faction != dest->faction && src->faction->age < GiveRestriction()) {
		if (ord!=NULL) {
			ADDMSG(&src->faction->msgs, msg_feedback(src, ord, "giverestriction",
				"turns", GiveRestriction()));
		}
		return -1;
	} else if (n == 0) {
		error = 36;
	} else if (itype->flags & ITF_CURSED) {
		error = 25;
	} else if (itype->give && !itype->give(src, dest, itype, n, ord)) {
		return -1;
	} else {
		int use = new_use_pooled(src, item2resource(itype), GET_SLACK, n);
		if (use<n) use += new_use_pooled(src, item2resource(itype), GET_RESERVE|GET_POOLED_SLACK, n-use);
		if (dest) {
			u_changeitem(dest, itype, n);
#if RESERVE_DONATIONS
			new_change_resvalue(dest, item2resource(itype), n);
#elif RESERVE_GIVE
			if (src->faction==dest->faction) {
				new_change_resvalue(dest, item2resource(itype), n);
			}
#endif
			handle_event(&src->attribs, "give", dest);
			handle_event(&dest->attribs, "receive", src);
#if defined(MUSEUM_MODULE) && defined(TODO)
TODO: Einen Trigger benutzen!
				if (a_find(dest->attribs, &at_warden)) {
					/* warden_add_give(src, dest, itype, n); */
				}
#endif
		} else {
			/* gib bauern */
			give_peasants(use, itype, src);
		}
	}
	add_give(src, dest, n, item2resource(itype), ord, error);
	if (error) return -1;
	return 0;
}

void
givemen(int n, unit * u, unit * u2, struct order * ord)
{
	struct ship *sh;
	int error = 0;

	if (u2 && ufaction(u) != ufaction(u2) && ufaction(u)->age < GiveRestriction()) {
		ADDMSG(&ufaction(u)->msgs, msg_feedback(u, ord, "giverestriction",
			"turns", GiveRestriction()));
		return;
	}
	if (u2 && u2->number) {
		/* kann keine personen an eine nicht-leere einheit geben */
		error = 155;
	} else if (fval(u, UFL_HERO) || (u2 && fval(u2, UFL_HERO))) {
		/* heroes cannot be passed on */
		error = 158;
	} else if (u == u2) {
		error = 10;
	} else if (!u2 && u->race == new_race[RC_SNOTLING]) {
		/* Snotlings können nicht an Bauern übergeben werden. */
		error = 307;
	} else if ((u && unit_has_cursed_item(u)) || (u2 && unit_has_cursed_item(u2))) {
		error = 78;
	} else if (fval(u, UFL_LOCKED) || fval(u, UFL_HUNGER) || isslave(u)) {
		error = 74;
	} else if (u2 && (fval(u2, UFL_LOCKED) || isslave(u))) {
		error = 75;
	} else if (u2 != (unit *) NULL
		&& ufaction(u2) != ufaction(u)
		&& ucontact(u2, u) == 0) {
			error = 73;
		} else if (u2 && fval(u, UFL_WERE) != fval(u2, UFL_WERE)) {
			error = 312;
		} else {
			if (n > u->number) n = u->number;
			if (n == 0) {
				error = 96;
			} else if (u2 && ufaction(u) != ufaction(u2)) {
				if (ufaction(u2)->newbies + n > MAXNEWBIES) {
					error = 129;
				} else if (u->race != ufaction(u2)->race) {
					if (ufaction(u2)->race != new_race[RC_HUMAN]) {
						error = 120;
					} else if (count_migrants(ufaction(u2)) + n > count_maxmigrants(ufaction(u2))) {
						error = 128;
					} else if (u2->number!=0) {
						error = 139;
					}
				}
			}
		}
		if (!error) {
			if (u2 && u2->number == 0) {
				set_racename(&u2->attribs, get_racename(u->attribs));
				u2->race = u->race;
				u2->irace = u->irace;
			} else if (u2 && u2->race != u->race) {
				error = 139;
			}
		}
		if (u2) {
			skill * sv;
			for (sv=u->skills;error==0 && sv!=u->skills+u->skill_size;++sv) {
				boolean sk2 = has_skill(u2, sv->id);
				if (!sk2 || ufaction(u2)!=ufaction(u)) {
					const skill_type * skt = skt_get(sv->id);
					int limit = skt->limit?skt->limit(ufaction(u2)):INT_MAX;
					if (limit!=INT_MAX) {
						int k = count_skill(ufaction(u2), sv->id);
						if (ufaction(u)!=ufaction(u2)) k+=n;
						if (!sk2) k+=u2->number;
						if (k>limit) error = 156;
					}
				}
			}
			for (sv=u2->skills;error==0 && sv!=u2->skills+u2->skill_size;++sv) {
				boolean sk1 = has_skill(u, sv->id);
				if (!sk1 || ufaction(u2)!=ufaction(u)) {
					const skill_type * skt = skt_get(sv->id);
					int limit = skt->limit?skt->limit(ufaction(u2)):INT_MAX;
					if (limit!=INT_MAX) {
						int k = count_skill(ufaction(u2), sv->id) + n;
						if (k>limit) error = 156;
					}
				}
			}
		}
		if (!error) {
			if (u2) {
				/* Einheiten von Schiffen können nicht NACH in von
				* Nicht-alliierten bewachten Regionen ausführen */
				sh = leftship(u);
				if (sh) set_leftship(u2, sh);

				transfermen(u, u2, n);
				if (ufaction(u) != ufaction(u2)) {
					ufaction(u2)->newbies += n;
				}
			} else {
				if (getunitpeasants) {
#ifdef ORCIFICATION
					if (u->race == new_race[RC_ORC] && !fval(uregion(u), RF_ORCIFIED)) {
						attrib *a = a_find(uregion(u)->attribs, &at_orcification);
						if (!a) a = a_add(&uregion(u)->attribs, a_new(&at_orcification));
						a->data.i += n;
					}
#endif
					transfermen(u, NULL, n);
				} else {
					error = 159;
				}
			}
		}
		addgive(u, u2, n, R_PERSON, ord, error);
}
