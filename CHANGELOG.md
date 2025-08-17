# 30.3
	- Opfere Kraft: Statt der Aura, die der Magier verliert, wird jetzt
	  immer die Aura angezeigt, die das Ziel erhalten hat.

# 30.2

    - Die Liste der struct bfaction in battle.c war unnötig, und ist
      durch ein stb_ds array con struct faction ersetzt worden.
    - Ein paar kleine Meldungen von Coverity adressiert.
	- KAUFE und VERKAUFE werden auch außerhalb von Schiffen durch BEWACHE
	  verhindert.

# 30.1

	- Feuerwand erzeugt curses in den beiden Regionen.
	- Feuerwand-Auflösung nicht mehr über aging borders implementiert.
	- age_borders Mechanismus ist damit überflüssig.
	- Dateiformat: Kein Magier mehr in wall_data
	- Fluch brechen, Antimagiekristal und Zerstoere Magie können Feuerwand auflösen
	- Fluch brechen benutzt jetzt den selben Code wie andere Antimagie

# 29.4

	- Weg der Bäume kostet auch bei resistenten Zielen [Bug 3051]
	- Plappermaul testet nicht doppelt auf Resistenz [Bug 3051]

# 28.3

	- Zauberkosten sind abhängig von der Zauberstufe, oder Effekt:
	- Ritual der Aufnahme: Kosten abhängig von der Anzahl betroffener Personen.
	- Bei Zeitdehnung wird die Dauer aufgerundet, (Zauberstufe+1)/2
	- Verstärkende Wirkungen von Ring der Macht und Magierturm bei Zaubern repariert, e.g. Zeitdehnung
	- Sog des Lebens respektiert die Magieresistenz der Zielregion

# 27.1

	- Neue Formel zur Vermischung von Talenten bei GIB PERSON und REKRUTIERE
	- Bauern in übervölkerten Regionen vermehren sich nicht
	- Samen wachsen nicht zu Schößlingen heran, wenn sie dadurch Bauern die Felder wegnehmen.
	- Schiffe nehmen durch Überladung keinen Schaden mehr
	- Dracoide werden rekrutiert wie andere Einheiten [2106]
	- Heldenreform

# 3.30.2

	- Anzeige des Schiffsschaden im Report aufrunden [2797]

# 3.30.1

	- Schiffsschaden von Flotte falsch berechnet [2760]

# 3.30

	- Flottenschaden wurde falsch berechnet [2795]
	- Magier bekam wöchentlich einen Tiegel mit Krötenschleim [2783]
	- Magier blieben zu lange Kröte [2766]
	- Schiffsschaden von Flotte falsch berechnet [2760]
	- Nicht sichtbare Einheit wurde erfolgreich ausspioniert [2776]
	- Insekten hungern in Gletscher trotz Kälteschutz [2778]
	- Immer noch Schlümpfe unter den Dämonen [2779]
	- Banner wird nicht gelöscht [2774]

# 3.29

	- Seeschlange konnte nicht attackiert werden obwohl sie gesehen wird [2763]
	- Flucht auf Schiffen erlauben [2764]
	- Befehle wurden wegen non-breaking spaces ignoriert [2753]
	- Portale haben alle Einheiten in die selbe Zielregion geschickt [2738]
	- Noch eine Reparatur für Langzeitschlümpfe [2758]
 
# 3.28

	- Bugfix für Schild des Fisches (Solthar)
	- Dämonen können magisch reanimiert werden.
	- "Schöne Träume" verliert seine Wirkung, wenn der Zauberer stirbt.
	- Mit GIB 0 können hungernde Personen an die Bauern gegeben werden.
	- Magieresistenz: Einheiten widerstehen nicht Zaubern der eigenen Partei [2733].
	- Zauberkosten steigen durch Ring der Macht nicht an.
	- Effektiv gezauberte Stufe von Zauber anhängig von Verfügbarkeit der Materialen.
	- Ring der Macht und Steinkreis erhöhen nicht die Zauberkosten [2737].
	- Limits für Vertrautenzauber korrekt implementiert.
	- Kröten und Schlümpfe können nichts lernen.

# 3.27

	- Schiffe sind kommentarlos nicht gesegelt [2722].
	- Meermenschen konnten nicht mehr anschwimmen [2723].
	- Magieresistenz repariert [2724].
	- Kleine Änderung an Samenwachstum.
	- Umstellung auf neue Versionen von externen Libraries.

# 3.26

	- Akademien, Traenke und Verzauberungen wirken auch bei LERNE AUTO
	- Das lernen in einer Akademie erhoeht die Lernkosten. Koennen diese
	nicht bezahlt werden, wird ohne deren Bonus gelernt.
	- Lehrer muessen nicht mehr in der Akademie stehen, damit ihre Schueler
	den Bonus bekommen
	- Rohstoffe koennen jetzt bereits gesehen werden, wenn eine Einheit nur
	die Haelfte des zum Abbau noetigen Talentes hat (statt bisher
	Talent-1)
	- Mauern der Ewigkeit und Störe Astrale Integrität brauchen keine
	Stufenangabe, ihre Kosten sind nicht variabel [2651]

# 3.25

	- Ab sofort ist es nicht mehr erlaubt, Befehle mit weniger als 3 
	Zeichen abzukürzen.
	- Leuchttürme entdecken Seeschlangen und Drachen auf dem Ozean [2688]
	- Magieresistenz von Insekten und Goblins repariert [2685]
	- Getarnte Einheiten können wieder Eisen abbauen [2679]
	- Gestaltwandlung kann nur einmal auf die selbe Einheit wirken [2680] 
	- Handel benötigt eine Burg mit Mindestgröße 2 [2678]
	- Geschützte Leerzeichen in Befehlen werden ignoriert [2670]

# 3.12

	- [bug] Einheitenlimit bei GIB PERSON beachten
	- [bug] Einheitenlimit bei ALLIANCE JOIN beachten
	- [rule] Einheiten- und Personenzahl im Report beinhaltet *alle* Einheiten der Partei.
	- [other] Statistik fuer Spielleiter zeigt das Parteisilber nicht mehr an.
	- [other] Berechnung der Message-Ids optimiert.


