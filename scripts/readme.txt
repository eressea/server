Status und Statusänderungen, USERS.STATUS

WAITING: Initialer Status, warten auf Bestätigung der Anmeldung. 
Übergänge: 
-> CONFIRMED, wenn Anmeldung von Benutzer bestätigt wurde.
-> EXPIRED, falls bis zum ZAT keine Bestätigung eintraf.

CONFIRMED: Emailadresse des Spielers ist korrekt, seine Anmeldung wurde in
der laufenden Woche bestätigt.
Übergänge:
-> WAITING, wenn er zum ZAT nicht ausgesetzt wurde
-> ACTIVE, wenn er einen Report bekommen hat

ACTIVE: Spieler ist aktiv.
Übergänge: (derzeit keine)

INVALID: Spieler hat ungültige Daten übermittelt

BANNED: Spieler ist aus dem Spiel ausgeschlossen worden.
