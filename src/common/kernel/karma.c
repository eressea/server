/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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
#include "save.h"
#include "race.h"
#include "region.h"
#include "item.h"
#include "group.h"
#include "order.h"
#include "pool.h"
#include "skill.h"
#include "faction.h"
#include "magic.h"
#include "message.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/rng.h>

/* libc includes */
#include <math.h>
#include <stdlib.h>

attrib_type at_faction_special = {
	"faction_special", NULL, NULL, NULL, a_writeshorts, a_readshorts
};

int
age_prayer_timeout(attrib *a) {
	return --a->data.sa[0];
}

attrib_type at_prayer_timeout = {
	"prayer_timeout", NULL, NULL, age_prayer_timeout, a_writeshorts, a_readshorts
};

attrib_type at_prayer_effect = {
	"prayer_effect", NULL, NULL, NULL, NULL, NULL
};

attrib_type at_wyrm = {
	"wyrm", NULL, NULL, NULL, a_writeint, a_readint
};

attrib_type at_fshidden = {
	"fshidden", NULL, NULL, NULL, a_writeint, a_readint
};

attrib_type at_jihad = {
	"jihad", NULL, NULL, NULL, a_writeshorts, a_readshorts
};

#ifdef KARMA_MODULE
struct fspecialdata fspecials[MAXFACTIONSPECIALS] = {
	{
		"Regeneration",
		"Personen in einer Partei mit dieser Eigenschaft heilen jeden "
		"Schaden innerhalb einer Woche und zusätzlich in jeder Kampfrunde "
		"HP entsprechend ihres Ausdauer-Talents. Sie benötigen jedoch 11 "
		"Silber Unterhalt pro Woche.",
		1
	},
	{ /* TODO: Für alte Parteien zu stark */
		"Städter",
		"Personen einer Partei mit dieser Eigenschaft lieben die Städte und "
		"verabscheuen das Leben in der freien Natur. Ihr Arbeitlohn verändert "
		"sich in Abhängigkeit vom größten Gebäude in der Region. Ist es eine "
		"Zitadelle so erhalten sie 2 Silber mehr pro Runde, bei einer Festung "
		"1 Silber mehr. Bei einer Burg bekommen sie den normalen Arbeitslohn, "
		"bei einem Turm 1 Silber, bei einer Befestigung 3 Silber weniger. Gibt "
		"es kein entsprechendes Gebäude in der Region, verringert sich ihr "
		"Arbeitslohn um 5 Silber.",
		1
	},
	{
		"Barbar",
		"Einheiten dieser Partei erhalten durch Lernen von Waffentalenten "
		"(Taktik und Reiten zählen nicht dazu!) 40 statt 30 Lerntage. Weitere "
		"Stufen erhöhen den Bonus um 5 Lerntage. Die Fokussierung auf das "
		"Kriegerdasein führt jedoch zu Problemen, andere Talente zu erlernen. "
		"In allen nichtkriegerischen Talenten einschließlich Magie und Taktik "
		"erhalten sie die entsprechende Anzahl von Lerntagen weniger pro "
		"Lernwoche.",
		100
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
		100
	},
	/* TODO: Relativ sinnlose Eigenschaft. */
	{
		"Miliz",
		"Alle Personen dieser Partei beginnen mit 30 Talenttagen in allen "
		"Waffentalenten, in denen ihre Rasse keinen Malus hat. Zusätzliche "
		"Stufen bringen jeweils einen zusätzlichen Talentpunkt.",
		100
	},
	{ /* Ohne Schiffsunterhaltskosten schwache Eigenschaft. */
		/* Aufpassen: Tragkraft? */
		"Feenreich",
		"Alle Personen dieser Partei wiegen nur noch die Hälfte ihres normalen "
		"Gewichtes. Die Nähe zum Feenreich macht sie jedoch besonders "
		"anfällig gegen Waffen aus Eisen, bei einem Treffer durch eine "
		"solche Waffe nehmen sie einen zusätzlichen Schadenspunkt. Zusätzliche "
		"Stufen dieser Eigenschaft verringern das Gewicht auf 1/3, 1/4, ... und "
		"erhöhen den Schaden durch Eisenwaffen um einen weiteren Punkt.",
		100
	},
	{
		"Administrator",
		"Das Einheitenlimit einer Partei mit dieser Eigenschaft erhöht sich um "
		"400 Einheiten. Leider verschlingt der Verwaltungsaufwand viel Silber, "
		"so dass sich der Unterhalt pro Person um 1 Silberstück erhöht. Weitere "
		"Stufen der Eigenschaft erhöhen das Limit um weitere 400 Einheiten und "
		"den Unterhalt um ein weiteres Silberstück.",
		100
	},
	/* TODO: Noch nicht so implementiert */
	{
		"Telepath",
		"Der Geist fremder Personen bleibt den Magiern einer Partei mit dieser "
		"Eigenschaft nicht verschlossen. Fremde Einheiten erscheinen im Report "
		"wie eigene Einheiten, mit allen Talentwerten und Gegenständen. Leider "
		"führt eine so intensive Beschäftigung mit dem Geistigen zur körperlichen "
		"Verkümmerung, und die Partei erhält -1 auf alle Talente außer "
		"Alchemie, Kräuterkunde, Magie, Spionage, Tarnung und Wahrnehmung. Wird "
		"diese Eigenschaft ein zweites Mal erworben, so bleibt die Wirkung nicht "
		"auf Magier beschränkt, sondern alle Einheiten einer Partei können "
		"die Talente und Gegenstände aller fremden Einheiten sehen. Allerdings "
		"gibt es in beiden Fällen eine Einschränkung: Die Einheit muß sich einen "
		"Monat lang auf die psychischen Ströme einer Region einstellen, bevor "
		"sie in der Lage ist, die Gedanken der anderen zu lesen.",
		2
	},
	{
		"Amphibium",
		"Einheiten dieser Partei können durch Ozeanfelder laufen. Allerdings "
		"nehmen sie 10 Schadenspunkte, wenn sie am Ende einer Woche in einem "
		"Ozeanfeld stehen. Pferde weigern sich, in ein Ozeanfeld zu laufen. "
		"Zusätzliche Stufen dieser Eigenschaft reduzieren den Schaden um jeweils "
		"5 Punkte. Achtung: Auf dem Ozean wird kein Schaden regeneriert.",
		3
	},
	/* TODO: negative Eigenschaft */
	{
		"Magokrat",
		"Eine Partei mit dieser Eigenschaft hat eine so hohe magische "
		"Affinität, dass sie pro Stufe der Eigenschaft zwei zusätzlich "
		"Magier ausbilden kann.",
		100
	},
	/* TODO: negative Eigenschaft, vergleichsweise schwach */
	{
		"Sappeur",
		"Befestigungen wirken gegen Einheiten einer Partei mit dieser "
		"Eigenschaft nur mit ihrer halben Schutzwirkung (aufgerundet).",
		1
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
		100
	},
	{
		/* Evt. zwei Stufen draus machen */
		"Versteckt",
		"Eine Partei mit dieser Eigenschaft hat die Fähigkeit zu Tarnung "
		"zur Perfektion getrieben. Jede Einheit mit mindestens Tarnung 3 "
		"versteckt alle ihre Gegenstände so, daß sie von anderen Parteien "
		"nicht mehr gesehen werden können. Jede Einheit mit mindestens Tarnung 6 "
		"schafft es auch, die Zahl der in ihr befindlichen Personen zu verbergen. "
		"Um diese Eigenschaft steuern zu können, stehen diesen Parteien die "
		"Befehle TARNE ANZAHL [NICHT] und TARNE GEGENSTÄNDE [NICHT] zur "
		"Verfügung.",
		1
	},
	/* TODO: Noch nicht implementiert */
	{
		"Erdelementarist",
		"Alle Gebäude dieser Partei sind von Erdelementaren beseelt und können "
		"sich mit Hilfe eines speziellen Befehls jede Woche um eine Region "
		"bewegen. Dies macht es den Bewohnern - welche alle einer Partei mit "
		"dieser Eigenschaft angehören müssen - jedoch unmöglich, in dieser "
		"Woche ihren normalen Tätigkeiten nachzugehen.",
		1
	},
	{
		"Magische Immunität",
		"Eine Partei mit dieser Eigenschaft ist völlig immun gegen alle Arten "
		"von Magie. Allerdings verlieren die Magier einer solchen Partei ihre "
		"Fähigkeit, Aura zu regenerieren, völlig.",
		1
	},
	/* TODO: Noch nicht implementiert */
	{
		"Vielseitige Magie",
		"Eine Partei mit dieser Eigenschaft kann einen ihrer Magier in einem "
		"anderen als dem Parteimagiegebiet ausbilden. Weitere Stufen ermöglichen "
		"jeweils einem weiteren Magier das Lernen eines anderen Gebiets.",
		100
	},
	{
		"Jihad",
		"Eine Partei mit dieser Eigenschaft kann eine (Spieler)-Rasse "
		"mit dem speziellen kurzen Befehl JIHAD <RASSE> zum Feind erklären. "
		"Bei einem Kampf gegen einen Angehörigen dieser Rasse bekommen ihre "
		"Kämpfer grundsätzlich einen Bonus von +1 auf Angriff und Schaden. "
		"Allerdings kann es zu spontanen Pogromen gegen Angehörige der mit einem "
		"Jihad belegten Rasse kommen. Wird die Eigenschaft mehrmals erworben "
		"können entweder mehrere Rassen mit einem Jihad belegt werden, "
		"oder eine Rasse mehrfach, in diesem Fall addiert sich die Wirkung. "
		"Ein einmal erklärter Jihad kann nicht wieder rückgängig gemacht "
		"werden.",
		100
	},
	/* TODO: is_undead() und Sonderbehandlungen von Untoten */
	/*       in dieser Form ultimative Eigenschaft für Orks */
	{
		"Untot",
		"Personen einer Partei mit dieser Eigenschaft bekommen automatisch doppelt "
		"soviele Trefferpunkte wie normale Angehörige der entsprechenden Rasse, "
		"verlieren jedoch ihre Fähigkeit zur Regeneration erlittenen Schadens "
		"komplett.",
		1
	},
	{
		"Windvolk",
		"Ein solches Volk ist sehr athletisch und die Kinder üben besonders "
		"den Langstreckenlauf von kleinauf. Nach jahrelangem Training sind "
		"sie dann in der Lage, sich zu Fuß so schnell zu bewegen als würden "
		"sie reiten. Allerdings hat das jahrelange Konditionstraining ihre "
		"Kräfte schwinden lassen, und ihre Tragkraft ist um 2 Gewichtseinheiten "
		"verringert.",
		1
	},
	{
		"Glück",
		"Diese Eigenschaft bewirkt, das der Partei gelegentlich positive "
		"Ereignisse zustoßen. Dies können spontane Verbesserungen des "
		"Wissensstandes, ein Fund wertvoller Gegenstände oder ähnliches "
		"sein. Je häufiger diese Eigenschaft erworben wird, desto größer die "
		"Wahrscheinlichkeit für solche Ereignisse, und desto postiver ihre "
		"Auswirkungen.",
		100
	},
	{
		"Lykanthrop",
		"Angehörige einer Partei mit dieser Eigenschaft sind Werwesen. Einheiten "
		"einer solchen Partei können sich mit Hilfe der langen Befehle 'WERWESEN' und "
		"'WERWESEN NICHT' in eine andere Form verwandeln. Beide Befehle haben "
		"nur eine gewisse Erfolgswahrscheinlichkeit und funktionieren nicht immer: "
		"Je höher die Stufe der Eigenschaft, desto größer die Wahrscheinlichkeit, "
		"dass die Verwandlung gelingt, und je geringer die Chance, sich "
		"zurückzuverwandeln. Zudem besteht eine zunehmende kleine Chance der "
		"spontanen Verwandlung. In Werform erhalten die Einheiten einen Bonus auf "
		"Angriff, Schaden und natürliche Rüstung in Höhe der Eigenschaft. Sie "
		"benutzen jedoch keine Pferde im Kampf, und verwenden als Rüstungen nur "
		"Schilde. Sie sind eingeschränkt und können in Werform kein Geld verdienen, "
		"nicht Zaubern und nicht Lernen oder Lehren.",
		4
	},
	/* TODO: Noch nicht implementiert */
	{
		"Elite",
		"Für eine Partei mit dieser Eigenschaft verdoppeln sich alle Boni- und "
		"Mali ihrer Rasse. Ihre Unterhaltskosten erhöhen sich auf 12 Silber pro "
		"Runde, ihr Einheitenlimit reduziert sich auf 25%%. Diese Eigenschaft "
		"kann nicht erworben werden, wenn die Partei nach dem Erwerb zuviele "
		"Einheiten hätte.",
		1
	}
};

void
buy_special(unit *u, struct order * ord, fspecial_t special)
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
		cmistake(u, ord, 250, MSG_EVENT);
		return;
	}

	/* Alles ok, attribut geben */

	if (a2) {
		if(a2->data.sa[1] < fspecials[special].maxlevel) {
			a2->data.sa[1]++;
      ADDMSG(&f->msgs, msg_message("new_fspecial_level", 
        "special level", special, a2->data.sa[1]));
		} else {
			cmistake(u, ord, 251, MSG_EVENT);
			return;
		}
	} else {
		a2 = a_add(&f->attribs, a_new(&at_faction_special));
		a2->data.sa[0] = (short)special;
		a2->data.sa[1] = 1;
		ADDMSG(&f->msgs, msg_message("new_fspecial", "special", special));
	}
}

int
fspecial(const faction *f, fspecial_t special)
{
  attrib *a;

  for (a=a_find(f->attribs, &at_faction_special); a; a=a->nexttype) {
    if(a->data.sa[0] == special) return a->data.sa[1];
  }
  return 0;
}

static int
sacrifice_cmd(unit * u, struct order * ord)
{
  int   n = 1, karma;
  const char *s;
  
  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (s && *s) n = atoi(s);
  if (n <= 0) {
    cmistake(u, ord, 252, MSG_EVENT);
    return 0;
  }

  s = getstrtoken();

  switch(findparam(s, u->faction->locale)) {
  case P_SILVER:
    n = use_pooled(u, oldresourcetype[R_SILVER], GET_DEFAULT, n);
    if(n < 10000) {
      cmistake(u, ord, 51, MSG_EVENT);
      return 0;
    }
    change_resource(u, oldresourcetype[R_SILVER], n);
    karma = n/10000;
    u->faction->karma += karma;
    break;

  case P_AURA:
    if(!is_mage(u)) {
      cmistake(u, ord, 214, MSG_EVENT);
      return 0;
    }
    if (get_level(u, SK_MAGIC) < 10) {
      cmistake(u, ord, 253, MSG_EVENT);
      return 0;
    }
    n = min(get_spellpoints(u), min(max_spellpoints(u->region, u), n));
    if(n <= 0) {
      cmistake(u, ord, 254, MSG_EVENT);
      return 0;
    }
    karma = n;
    u->faction->karma += n;
    change_maxspellpoints(u, -n);
    break;
  default:
    cmistake(u, u->thisorder, 255, MSG_EVENT);
  }
  return 0;
}

static int
prayer_cmd(unit * u, struct order * ord)
{
  region *r = u->region;
  attrib *a, *a2;
  unit *u2;
  int karma_cost;
  short mult = 1;
  param_t p;
  const char *s;
  
  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (findparam(s, u->faction->locale) == P_FOR) s = getstrtoken();
  p = findparam(s, u->faction->locale);

  switch(p) {
  case P_AURA:
    if (!is_mage(u)) {
      cmistake(u, ord, 214, MSG_EVENT);
      return 0;
    }
  case P_AID:
  case P_MERCY:
    break;
  default:
    cmistake(u, ord, 256, MSG_EVENT);
    return 0;
  }

  a = a_find(u->faction->attribs, &at_prayer_timeout);
  if (a) mult = (short)(2 * a->data.sa[1]);
  karma_cost = 10 * mult;

  if (u->faction->karma < karma_cost) {
    cmistake(u, ord, 250, MSG_EVENT);
    return 0;
  }

  u->faction->karma -= karma_cost;

  ADDMSG(&u->faction->msgs, msg_message("pray_success", "unit", u));

  switch (p) {
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
  return 0;
}

static int
jihad_cmd(unit * u, struct order * ord)
{
  faction *f = u->faction;
  int can = fspecial(f, FS_JIHAD);
  int has = 0;
  const race * jrace;
  race_t jrt;
  attrib *a;
  const char *s;

  for (a = a_find(f->attribs, &at_jihad); a; a = a->nexttype) {
    has += a->data.sa[1];
  }

  if (has >= can) {
    cmistake(u, ord, 280, MSG_EVENT);
    return 0;
  }

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (!s || !*s) {
    cmistake(u, ord, 281, MSG_EVENT);
    return 0;
  }

  jrace = rc_find(s);
  jrt = old_race(jrace);

  if (!playerrace(jrace)) {
    cmistake(u, ord, 282, MSG_EVENT);
    return 0;
  }

  for (a = a_find(f->attribs, &at_jihad); a; a = a->nexttype) {
    if (a->data.sa[0] == jrt) break;
  }

  if (a) {
    a->data.sa[1]++;
  } else {
    a = a_add(&f->attribs, a_new(&at_jihad));
    a->data.sa[0] = (short)jrt;
    a->data.sa[1] = 1;
  }

  add_message(&f->msgs, msg_message("setjihad", "race", jrace));
  return 0;
}

int
jihad(faction *f, const race * rc)
{
	attrib *a;
	race_t jrt = old_race(rc);

	for(a = a_find(f->attribs, &at_jihad); a; a = a->nexttype) {
		if(a->data.sa[0] == jrt) return a->data.sa[1];
	}
	return 0;
}

void
jihad_attacks(void)
{
	faction *f;
	region *r;
	unit *u, *u2;
	ally *sf, **sfp;

	for(f=factions; f; f=f->next) if(fspecial(f, FS_JIHAD)) {
    region * last = lastregion(f);
		for (r=firstregion(f); r != last; r = r->next) if (rng_int()%1000 <= 1) {
			boolean doit = false;

			for(u=r->units; u; u=u->next) if(jihad(f, u->race)) {
				doit = true;
				break;
			}

			if(doit == false) continue;

			log_printf("-->> Pogrom durch %s in %s\n", factionid(f), regionname(r, NULL));

			for(u2 = r->units; u; u=u->next) if(u2->faction == f) {
				for(u=r->units; u; u=u->next) if(jihad(f, u->race)) {
					/* Allianz auflösen */
					sfp = &u2->faction->allies;
          if (fval(u, UFL_GROUP)) {
  					attrib * a = a_find(u2->attribs, &at_group);
	  				if (a) sfp = &((group*)a->data.v)->allies;
          }

					for (sf=*sfp; sf; sf = sf->next) {
						if(sf->faction == u->faction) break;
					}

					if (sf) sf->status = sf->status & (HELP_ALL - HELP_FIGHT);

					sprintf(buf, "%s %s", locale_string(u->faction->locale, keywords[K_ATTACK]), unitid(u));
					addlist(&u2->orders, parse_order(buf, u->faction->locale));
				}
			}
		}
	}
}

void
karma(void)
{
  parse(K_PRAY, prayer_cmd, false);
  parse(K_SETJIHAD, jihad_cmd, false);
  parse(K_SACRIFICE, sacrifice_cmd, true);
}
#endif
