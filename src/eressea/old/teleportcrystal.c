/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/
#ifdef COMPATIBILITY
/* ------------------------------------------------------------- */
void
use_teleportcrystal(region * r, unit * mage, strlist * cmdstrings)
{
	region *target_region = NULL;
	unit *target_unit;
	strlist *S;
	target_unit = getunit(r, mage);

	if (target_unit == NULL) {
		cmistake(mage, cmdstrings->s, 64, MSG_EVENT);
		return;
	}
	target_region = findunitregion(target_unit);

	if (target_unit->faction != mage->faction) {
		int kontaktiert = 0;

		/* Nun kommt etwas reichlich krankes, um den KONTAKTIERE-Befehl
		 * des Ziels zu überprüfen. */

		if(allied(target_unit, mage->faction, HELP_FIGHT)) {
			kontaktiert = 1;
		} else {
			for (S = target_unit->orders; S; S = S->next) {
				if (strncasecmp("KON", S->s, 3) == 0) {
					char *c;
					int kontakt = -1;
					/* Soweit, sogut. S->s ist also ein
						 * KONTAKTIERE. Nun gilt es, herauszufinden,
					 * wer kontaktiert wird. Das ist nicht trivial. */

					/* Zuerst muß der Parameter herausoperiert
					 * werden. */

					/* Leerzeichen finden */

					for (c = S->s; *c != 0; c++) {
						if (isspace((int)*c) != 0) {
							break;
						}
					}

					/* Wenn ein Leerzeichen da ist, ist *c != 0 und
					 * zeigt auf das Leerzeichen. */

					if (*c == 0) {
						continue;
					}
					kontakt = atoi(c);

					if (kontakt == mage->no) {
						kontaktiert = 1;
						break;
					}
				}
			}
		}

		if (kontaktiert == 0) {
			/* Fehler: "Die Ziel-Einheit hat keinen Kontakt mit uns
			 * aufgenommen" */
			cmistake(mage, cmdstrings->s, 72, MSG_EVENT);
			return;
		}

		if (!can_survive(target_unit, r)) {
			cmistake(mage, cmdstrings->s, 231, MSG_EVENT);
			return;
		}

		/* Zahl prüfen */

		if (get_item(mage, I_TELEPORTCRYSTAL) < target_unit->number) {
			/* Fehler: "Die Einheit hat nicht mehr genug Kristalle fuer so
			 * viele Personen" */
			cmistake(mage, cmdstrings->s, 141, MSG_EVENT);
			return;
		}
		/* Kristalle abziehen */

		set_item(mage, I_TELEPORTCRYSTAL,
		 get_item(mage, I_TELEPORTCRYSTAL) - target_unit->number);

		/* Einheit verschieben. Diesmal ohne großen Aufwand, da ja
		 * immer nur ans Ende der aktuellen Region angehängt werden
		 * kann. */

		move_unit(target_unit, r, NULL);

		/* Langen Befehl der Einheit löschen, sonst sind Mehrfachzauber
		 * möglich */

		set_string(&target_unit->thisorder, "");

		/* Meldung an die Parteien */

		add_message(&mage->faction->msgs, new_message(mage->faction,
			"teleport_success%u:unit%r:source%r:target", target_unit, target_region, r));
		if (target_unit->faction != mage->faction) {
			add_message(&target_unit->faction->msgs, new_message(target_unit->faction,
				"teleport_success%u:unit%r:source%r:target", target_unit, target_region, r));
		}
	}
}
#endif
