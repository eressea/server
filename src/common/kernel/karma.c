/* vi: set ts=2:
 *
 *	$Id: karma.c,v 1.3 2001/02/28 18:25:25 corwin Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

/* TODO: enum auf fst_ umstellen. Pointer auf Display-Routine */
#include <config.h>
#include "eressea.h"
#include "karma.h"

/* kernel includes */
#include "unit.h"
#include "race.h"
#include "region.h"
#include "item.h"
#include "group.h"
#include "pool.h"
#include "skill.h"
#include "faction.h"
#include "magic.h"
#include "message.h"

/* util includes */
#include <attrib.h>
#include <base36.h>

/* libc includes */
#include <math.h>
#include <stdlib.h>

attrib_type at_faction_special = {
	"faction_special", NULL, NULL, NULL, a_writedefault, a_readdefault
};

int
age_prayer_timeout(attrib *a) {
	return --a->data.sa[0];
}

attrib_type at_prayer_timeout = {
	"prayer_timeout", NULL, NULL, age_prayer_timeout, a_writedefault, a_readdefault
};

attrib_type at_prayer_effect = {
	"prayer_effect", NULL, NULL, NULL, NULL, NULL
};

attrib_type at_wyrm = {
	"wyrm", NULL, NULL, NULL, a_writedefault, a_readdefault
};

attrib_type at_fshidden = {
	"fshidden", NULL, NULL, NULL, a_writedefault, a_readdefault
};

attrib_type at_jihad = {
	"jihad", NULL, NULL, NULL, a_writedefault, a_readdefault
};

struct fspecialdata fspecials[MAXFACTIONSPECIALS] = {
	{
		"Regeneration",
		"Personen in einer Partei mit dieser Eigenschaft heilen jeden "
		"Schaden innerhalb einer Woche und zusätzlich in jeder Kampfrunde "
		"HP entsprechend ihres Ausdauer-Talents. Sie benötigen jedoch 11 "
		"Unterhalt pro Woche.",
		false
	},
	{
		"Städter",
		"Personen einer Partei mit dieser Eigenschaft lieben die Städte und "
		"verabscheuen das Leben in der freien Natur. Ihr Arbeitlohn verändert "
		"sich in Abhängigkeit vom größten Gebäude in der Region. Ist es eine "
		"Zitadelle so erhalten sie 2 Silber mehr pro Runde, bei einer Festung "
		"1 Silber mehr. Bei einer Burg bekommen sie den normalen Arbeitslohn, "
		"bei einem Turm 1 Silber, bei einer Befestigung 3 Silber weniger. Gibt "
		"es kein entsprechendes Gebäude in der Region, verringert sich ihr "
		"Arbeitslohn um 5 Silber.",
		false
	},
	{
		"Krieger",
		"Einheiten dieser Partei erhalten durch Lernen von Waffentalenten "
		"(Taktik und Reiten zählen nicht dazu!) 40 statt 30 Lerntage. Weitere "
		"Stufen erhöhen den Bonus um +5/+10. Die Fokussierung auf das "
		"Kriegerdasein führt jedoch zu Problemen, andere Talente zu erlernen. "
		"In allen nichtkriegerischen Talenten einschließlich Magie und Taktik "
		"erhalten sie die entsprechende Anzahl von Lerntagen weniger pro "
		"Lernwoche.",
		true
	},
	/* TODO: Noch nicht so implementiert. */
	{
		"Wyrm",
		"Eine Partei mit dieser Eigenschaft kann einen ihrer Magier mit Hilfe "
		"eines speziellen Zaubers permanent in einen Drachen verwandeln. Der "
		"Drache ist zunächst jung, kann sich jedoch durch Lernen des speziellen "
		"Talents 'Drachenwesen' in einen größeren Drachen verwandeln. Um zu "
		"einem ausgewachsenen Drachen zu werden benötigt der verwandelte Magier "
		"die Talentstufe 6, um zu einem Wyrm zu werden die Talentstufe 12. "
		"Ein junger Drache benötigt 1000 Silber Unterhalt pro Woche, ein "
		"ausgewachsener Drache 5000 und ein Wyrm 10000 Silber. Bekommt er "
		"dieses Silber nicht, besteht eine Wahrscheinlichkeit, das er desertiert!",
		true
	},
	/* TODO: Relativ sinnlose Eigenschaft. */
	{
		"Miliz",
		"Alle Personen dieser Partei beginnen mit 30 Talenttagen in allen "
		"Waffentalenten, in denen ihre Rasse keinen Malus hat. Zusätzliche "
		"Stufen bringen jeweils einen zusätzlichen Talentpunkt.",
		true
	},
	{
		"Feenreich",
		"Alle Personen dieser Partei wiegen nur noch die Hälfte ihres normalen "
		"Gewichtes. Die Nähe zum Feenreich macht sie jedoch besonders "
		"anfällig gegen Waffen aus Eisen, bei einem Treffer durch eine "
		"solche Waffe nehmen sie einen zusätzlichen Schadenspunkt. Zusätzliche "
		"Stufen dieser Eigenschaft verringern das Gewicht auf 1/3, 1/4, ... und "
		"erhöhen den Schaden durch Eisenwaffen um einen weiteren Punkt.",
		true
	},
	{
		"Administrator",
		"Das Einheitenlimit einer Partei mit dieser Eigenschaft erhöht sich um "
		"250 Einheiten. Leider verschlingt der Verwaltungsaufwand viel Silber, "
		"so dass sich der Unterhalt pro Person um 1 Silberstück erhöht. Weitere "
		"Stufen der Eigenschaft erhöhen das Limit um weitere 250 Einheiten und "
		"den Unterhalt um ein weiteres Silberstück.",
		true
	},
	/* TODO: Noch nicht so implementiert */
	{
		"Telepath",
		"Der Geist fremder Personen bleibt den Magiern einer Partei mit dieser "
		"Eigenschaft nicht verschlossen. Fremde Einheiten erscheinen im Report "
		"wie eigene Einheiten, mit allen Talentwerten und Gegenständen. Leider "
		"führt eine so intensive Beschäftigung mit dem Geistigen zur körperlichen "
		"Verkümmerung, und die Partei erhält -1 auf alle Talente außer "
		"Alchemie, Kräuterkunde, Magie, Spionage, Tarnung und Wahrnehmung.",
		false
	},
	{
		"Amphibium",
		"Einheiten dieser Partei können durch Ozeanfelder laufen. Allerdings "
		"nehmen sie 10 Schadenspunkte, wenn sie am Ende einer Woche in einem "
		"Ozeanfeld stehen. Pferde weigern sich, in ein Ozeanfeld zu laufen. "
		"Zusätzliche Stufen dieser Eigenschaft reduzieren den Schaden um jeweils "
		"5 Punkte. Achtung: Auf dem Ozean wird kein Schaden regeneriert.",
		true
	},
	/* TODO: negative Eigenschaft */
	{
		"Magokrat",
		"Eine Partei mit dieser Eigenschaft hat eine so hohe magische "
		"Affinität, dass sie pro Stufe der Eigenschaft zwei zusätzlich Magier "
		"ausbilden kann.",
		true
	},
	/* TODO: negative Eigenschaft */
	{
		"Sappeur",
		"Befestigungen wirken gegen Einheiten einer Partei mit dieser "
		"Eigenschaft nur mit ihrer halben Schutzwirkung (aufgerundet).",
		false
	},
	/* TODO: Noch nicht implementiert */
	{
		"Springer",
		"Einheiten einer Partei mit dieser Eigenschaft können sich mit "
		"Hilfe eines speziellen Befehls über eine Entfernung von zwei "
		"Regionen teleportieren. Einheiten, die sich so teleportieren, "
		"bleiben jedoch für einige Wochen instabil und können sich in "
		"seltenen Fällen selbständig in eine zufällige Nachbarregion "
		"versetzen. Zusätzliche Stufen erhöhen die Reichweite um jeweils "
		"eine Region, erhöhen jedoch die Wahrscheinlichkeit eines zufälligen "
		"Versetzens geringfügig.",
		true
	},
	{
		"Versteckt",
		"Eine Partei mit dieser Eigenschaft hat die Fähigkeit zu Tarnung "
		"zur Perfektion getrieben. Jede Einheit mit mindestens Tarnung 2 "
		"versteckt alle ihre Gegenstände so, daß sie von anderen Parteien "
		"nicht mehr gesehen werden können. Jede Einheit mit mindestens Tarnung 5 "
		"schafft es auch, die Zahl der in ihr befindlichen Personen zu verbergen. "
		"Um diese Eigenschaft steuern zu können, stehen diesen Parteien die "
		"Befehle TARNE ANZAHL [NICHT] und TARNE GEGENSTÄNDE [NICHT] zur "
		"Verfügung.",
		false
	},
	/* TODO: Noch nicht implementiert */
	{
		"Erdelementarist",
		"Alle Gebäude dieser Partei sind von Erdelementaren beseelt und können "
		"sich mit Hilfe eines speziellen Befehls jede Woche um eine Region "
		"bewegen. Dies macht es den Bewohnern jedoch unmöglich, in dieser "
		"Woche ihren normalen Tätigkeiten nachzugehen.",
		false
	},
	/* TODO: Noch nicht implementiert */
	{
		"Magische Immunität",
		"Eine Partei mit dieser Eigenschaft ist völlig immun gegen alle Arten "
		"von Magie. Allerdings verlieren die Magier einer solchen Partei ihre "
		"Fähigkeit, Aura zu regenerieren, völlig.",
		false
	},
	/* TODO: Noch nicht implementiert */
	{
		"Vielseitige Magie",
		"Eine Partei mit dieser Eigenschaft kann einen ihrer Magier in einem "
		"anderen als dem Parteimagiegebiet ausbilden. Weitere Stufen ermöglichen "
		"jeweils einem weiteren Magier das Lernen eines anderen Gebiets.",
		true
	},
	/* TODO: Namen ändern, negative Eigenschaft implementieren */
	{
		"Jihad",
		"Eine Partei mit dieser Eigenschaft kann eine (Spieler)-Rasse "
		"mit dem speziellen kurzen Befehl JIHAD <RASSE> zum Feind erklären. "
		"Bei einem Kampf gegen einen Angehörigen dieser Rasse bekommen ihre "
		"Kämpfer grundsätzlich 1 auf Angriff und Schaden. Allerdings "
		"kann es zu spontanen Pogromen gegen Angehörige der mit einem "
		"Jihad belegten Rasse kommen. Wird die Eigenschaft mehrmals erworben "
		"können entweder mehrere Rassen mit einem Jihad belegt werden, "
		"oder eine Rasse mehrfach, in diesem Fall addiert sich die Wirkung. "
		"Ein einmal erklärter Jihad kann nicht wieder rückgängig gemacht "
		"werden.",
		true
	},
	/* TODO: is_undead() und Sonderbehandlungen von Untoten */
	{
		"Untot",
		"Personen einer Partei mit dieser Eigenschaft bekommen automatisch doppelt "
		"soviele Trefferpunkte wie normale Angehörige der entsprechenden Rasse, "
		"verlieren jedoch ihre Fähigkeit zur Regeneration erlittenen Schadens "
		"komplett.",
		false
	}
};

void
buy_special(unit *u, strlist *S, fspecial_t special)
{
	int count = 0;
	int cost;
	attrib *a, *a2 = NULL;
	faction *f = u->faction;

	/* Kosten berechnen */

	for(a=a_find(f->attribs, &at_faction_special); a; a=a->nexttype) {
		count += a->data.sa[1];
		if(a->data.sa[0] == special) a2 = a;
	}

	cost = (int)(100 * pow(3, count));

	/* Prüfen, ob genug Karma vorhanden. */

	if(f->karma < cost) {
		cmistake(u, S->s, 250, MSG_EVENT);
		return;
	}

	/* Alles ok, attribut geben */

	if(a2) {
		if(fspecials[special].levels) {
			a2->data.sa[1]++;
			add_message(&f->msgs, new_message(f,
				"new_fspecial_level%S:special%d:level", special, a2->data.sa[1]));
		} else {
			cmistake(u, S->s, 251, MSG_EVENT);
			return;
		}
	} else {
		a2 = a_add(&f->attribs, a_new(&at_faction_special));
		a2->data.sa[0] = (short)special;
		a2->data.sa[1] = 1;
		add_message(&f->msgs, new_message(f, "new_fspecial%S:special", special));
	}
}

int
fspecial(const faction *f, fspecial_t special)
{
	attrib *a;

	for(a=a_find(f->attribs, &at_faction_special); a; a=a->nexttype) {
		if(a->data.sa[0] == special) return a->data.sa[1];
	}

	return 0;
}

void
sacrificings(void)
{
	region *r;
	unit *u;

	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			if(igetkeyword(u->thisorder) == K_SACRIFICE) {
				int   n = 1, karma;
				char *s = getstrtoken();

				if(s && *s) n = atoi(s);
				if(n <= 0) {
					cmistake(u, u->thisorder, 252, MSG_EVENT);
					continue;
				}

				s = getstrtoken();

				switch(findparam(s)) {

				case P_SILVER:
					n = use_pooled(u, r, R_SILVER, n);
					if(n < 10000) {
						cmistake(u, u->thisorder, 51, MSG_EVENT);
						continue;
					}
					change_resource(u, R_SILVER, n);
					karma = n/10000;
					u->faction->karma += karma;
					break;

				case P_AURA:
					if(!is_mage(u)) {
						cmistake(u, u->thisorder, 214, MSG_EVENT);
						continue;
					}
					if(pure_skill(u, SK_MAGIC, u->region) < 10) {
						cmistake(u, u->thisorder, 253, MSG_EVENT);
						continue;
					}
					n = min(get_spellpoints(u), min(max_spellpoints(u->region, u), n));
					if(n <= 0) {
						cmistake(u, u->thisorder, 254, MSG_EVENT);
						continue;
					}
					karma = n;
					u->faction->karma += n;
					change_maxspellpoints(u, -n);
					break;
				default:
					cmistake(u, u->thisorder, 255, MSG_EVENT);
				}
			}
		}
	}
}

void
prayers(void)
{
	region *r;
	unit *u;
	strlist *S;

	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			for(S = u->orders; S; S = S->next) if(igetkeyword(S->s) == K_PRAY) {
				attrib *a, *a2;
				unit *u2;
				int karma_cost;
				short mult = 1;
				param_t p;
				char *s = getstrtoken();

				if(findparam(s) == P_FOR) s = getstrtoken();

				p = findparam(s);
				switch(p) {
				case P_AURA:
					if(!is_mage) {
						cmistake(u, S->s, 214, MSG_EVENT);
						continue;
					}
				case P_AID:
				case P_MERCY:
					break;
				default:
					cmistake(u, S->s, 256, MSG_EVENT);
					continue;
				}

				a = a_find(u->faction->attribs, &at_prayer_timeout);
				if (a) mult = (short)(2 * a->data.sa[1]);
				karma_cost = 10 * mult;

				if(u->faction->karma < karma_cost) {
					cmistake(u, S->s, 250, MSG_EVENT);
					continue;
				}

				u->faction->karma -= karma_cost;

				add_message(&u->faction->msgs, new_message(u->faction,
					"pray_success%u:unit",u));

				switch(p) {
				case P_AURA:
					set_spellpoints(u, max_spellpoints(u->region, u));
					break;
				case P_AID:
					for(u2 = r->units; u2; u2=u2->next) if(u2->faction == u->faction) {
						a2 = a_add(&u->attribs, a_new(&at_prayer_effect));
						a2->data.sa[0] = PR_AID;
						a2->data.sa[1] = 1;
					}
					break;
				case P_MERCY:
					for(u2 = r->units; u2; u2=u2->next) if(u2->faction == u->faction) {
						a2 = a_add(&u->attribs, a_new(&at_prayer_effect));
						a2->data.sa[0] = PR_MERCY;
						a2->data.sa[1] = 80;
					}
					break;
				}

				if(!a) a = a_add(&u->faction->attribs, a_new(&at_prayer_timeout));
				a->data.sa[0] = (short)(mult * 14);
				a->data.sa[1] = (short)mult;
			}
		}
	}
}

void
set_jihad(void)
{
	region *r;
	unit *u;
	strlist *S;

	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			for(S = u->orders; S; S=S->next) if(igetkeyword(S->s) == K_SETJIHAD) {
				faction *f = u->faction;
				int can = fspecial(f, FS_JIHAD);
				int has = 0;
				race_t jrace;
				attrib *a;
				char *s;

				for(a = a_find(f->attribs, &at_jihad); a; a = a->nexttype) {
					has += a->data.sa[1];
				}

				if(has >= can) {
					cmistake(u, S->s, 280, MSG_EVENT);
					continue;
				}

				s = getstrtoken();
				if(!s || !*s) {
					cmistake(u, S->s, 281, MSG_EVENT);
					continue;
				}

				jrace = findrace(s);

				if (race[jrace].nonplayer) {
					cmistake(u, S->s, 282, MSG_EVENT);
					continue;
				}
				
				for(a = a_find(f->attribs, &at_jihad); a; a = a->nexttype) {
					if(a->data.sa[0] == jrace) break;
				}

				if(a) {
					a->data.sa[1]++;
				} else {
					a = a_add(&f->attribs, a_new(&at_jihad));
					a->data.sa[0] = (short)jrace;
					a->data.sa[1] = 1;
				}

				add_message(&f->msgs, new_message(f,
					"setjihad%s:race", race[jrace].name[2]));
			}
		}
	}
}

int
jihad(faction *f, race_t race)
{
	attrib *a;

	for(a = a_find(f->attribs, &at_jihad); a; a = a->nexttype) {
		if(a->data.sa[0] == race) return a->data.sa[1];
	}

	return 0;
}

void
jihad_attacks(void)
{
	faction *f;
	region *r;
	unit *u, *u2;
	strlist *S;
	attrib *a;
	ally *sf, **sfp;
	
	for(f=factions; f; f=f->next) if(fspecial(f, FS_JIHAD)) {
		for(r=f->first; r != f->last; r = r->next) if(rand()%1000 <= 1) {
			boolean doit = false;

			for(u=r->units; u; u=u->next) if(jihad(f, u->race)) {
				doit = true;
				break;
			}

			if(doit == false) continue;

			printf("\tPogrom durch %s in %s\n", factionid(f), regionid(r));

			for(u2 = r->units; u; u=u->next) if(u2->faction == f) {
				for(u=r->units; u; u=u->next) if(jihad(f, u->race)) {
					/* Allianz auflösen */
					sfp = &u2->faction->allies;
					a = a_find(u2->attribs, &at_group);
					if (a) sfp = &((group*)a->data.v)->allies;

					for (sf=*sfp; sf; sf = sf->next) {
						if(sf->faction == u->faction) break;
					}

					if(sf) sf->status = sf->status & (HELP_ALL - HELP_FIGHT);

					sprintf(buf, "%s %s", keywords[K_ATTACK], unitid(u));
					S = makestrlist(buf);
					addlist(&u2->orders, S);
				}
			}
		}
	}
}
