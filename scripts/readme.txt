** Status und Statusänderungen

* USERS.STATUS

NEW: Initialer Status, warten auf Bestätigung der Anmeldung.
Übergänge: 
-> TUTORIAL, wenn Anmeldung von Benutzer bestätigt wurde.
-> INVALID, BANNED (nur manuell)

TUTORIAL: Emailadresse des Spielers ist korrekt, seine Anmeldung wurde
bestätigt, und er muss ein Tutorial bestehen.
Übergänge:
-> ACTIVE, wenn er ein Tutorial abgeschlossen hat
-> INVALID, BANNED (nur manuell)

ACTIVE: Spieler hat das Tutorial erfüllt, und kann sich für Partien anmelden
Übergänge:
-> INVALID, BANNED (nur manuell)

INVALID: Spieler hat ungültige Daten übermittelt

BANNED: Spieler ist aus dem Spiel ausgeschlossen worden.


* SUBSCRIPTIONS.STATUS

WAITING: Warten auf Bestätigung
-> EXPIRED
-> CONFIRMED

CONFIRMED: Bestätigung eingetroffen
-> WAITING
-> ACTIVE

ACTIVE: Spiel ist gestartet
-> DEAD
