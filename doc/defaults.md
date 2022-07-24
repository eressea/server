Ich habe natuerlich immer noch Fragen und Sonderfaelle 
Enno
 — 
## Mal ganz vorne anfangen.
Wenn in meiner Vorlage steht:
```
ARBEITE
// TODO: Kekse
```
und ich befehle diese Woche:
```
TREIBE
@BEWACHE
// Keine Kekse
GIB john 1 Keks
```
dann sollte in der neuen Vorlage stehen:
```
TREIBE
@BEWACHE
// Keine Kekse
```
Alte Befehle werden komplett ersetzt durch lange und permanente.
Das ist der einfache "Normalfall"

## Zweitens. Vorlage:
```
ARBEITE
@BEWACHE
// TODO Kekse
```

Befehle:
```
NACH W
// TODO Kekse
```

Neue Vorlage:
```
ARBEITE
// TODO Kekse
```
Wer das @BEWACHE behalten will, sollte es in den Befehlen drin lassen.

## Dritter Fall, Handel:
Vorlage:
```
KAUFE 100 Balsam
VERKAUFE ALLES Juwel
// Haendler
```

Befehle:
```
NACH W
// Haendler bewegen
```

Resultat:
```
KAUFE 100 Balsam
VERKAUFE ALLES Juwel
// Haendler bewegen
```

Also, bei NACH als langem Befehl aus den alten Befehlen die langen behalten, die Kommentare und anderen wiederholten Befehle löschen, und aus den neuen alle langen und wiederholten nehmen, die kein NACH sind.
Wenn der lange Befehl kein NACH ist, dann die alten Befehle komplett überschreiben mit den neuen langen, Kommentaren, und wiederholbaren. 
Mit Kommentaren meine ich hier //. Was mit ; passiert ist eh klar

Weil, Vorlage:
```
ARBEITE
// #call work
```

Neue Befehle:
```
UNTERHALTE
// #call entertain
```

Da will ich das // #call work gelöscht haben.
Wenn ich das nicht wollte, hätte ich es wieder eingeschickt.

Beim NMR bleiben die Kommentare bestehen, da wird ja nichts verändert.

Dem Server ist auch im NMR-Fall egal, was in den // Kommentaren steht 😉
Das waren jetzt die einfachen Fälle.

## Dann kommt der DEFAULT Befehl

Vorlage:
```
ARBEITE
// #call work
@BEWACHE
```

Neue Befehle:
```
LERNE Unterhaltung
DEFAULT UNTERHALTE
```

Resultat:
```
UNTERHALTE
```
Das ARBEITE wird ersetz vom LERNE, der Kommentar und BEWACHE werden gelöscht, weil ich sie nicht wiederholt habe, und das LERNE wird vom DEFAULT überschrieben.
Das ist quasi der einfache Fall.
Wenn mehrere DEFAULT Befehle, dann werden die alle behalten.
Also, Vorlage:
```
ARBEITE
```

Befehle:
```
TREIBE
@BEWACHE
DEFAULT UNTERHALTE
DEFAULT "@GIB 0 1 Person"
```

Resultat:
```
@BEWACHE
UNTERHALTE
@GIB 0 1 Person
```
 
Q: warum bleibt hier das @Bewache, oben aber nicht? bei #call work
A: Weil es oben in der Vorlage stand, du es dann aber aus den Befehlen gelöscht hast.

Also, soweit ist das immer noch nicht (zu) schwer, glaube ich.
## Nächster Fall. DEFAULT. Vorlage:
```
ARBEITE
@BEWACHE
```

Befehle:
```
UNTERHALTE
DEFAULT
// löscht alle Befehle
```

Resultat:
```
// löscht alle Befehle
```

Das mit `DEFAULT ""` ist ein Sonderfall, der uns noch Freude machen wird. 
Aber im einfachsten Falle funktionert das wie oben.
Obwohl ich glaube, aktuell löscht es auch den (neuen) Kommentar.
Zur Erinnerung, das war nötig, weil jemand seinem Untoten einen ZAUBERE Befehl gegeben hat. https://bugs.eressea.de/view.php?id=2843

Das ist glaube ich auch der Grund, warum DEFAULT am Ende der Auswertung bearbeitet wird, damit es die neu empfangenen Befehle mit löschen kann, nachdem sie ausgeführt wurden.
Ich hätte das gerne an den Anfang der Reihenfolge gelegt, aber sehe jetzt, dass das nicht gehen wird 😦

## Neuer Fall:
```
ZAUBERE "Drachen rufen"
```

Befehle:
```
KAUFE 1 Keks
DEFAULT
DEFAULT "VERKAUFE 1 Keks"
```

Ergebnis:
```
VERKAUFE 1 Keks
```
 
Q: Das "Default" tu hier genau nichts oder?
A: Ja. Aber DEFAULT-Befehle werden der Reihe nach abgearbeitet, das löscht also auch nicht das neue VERKAUFE.

### Gegenbeispiel:
```
ZAUBERE "Drachen rufen"
```

Befehle:
```
KAUFE 1 Keks
DEFAULT "VERKAUFE 1 Keks"
DEFAULT
DEFAULT "MACHE 1 Wunschpunsch"
```

Ergebnis:
```
MACHE 1 Wunschpunsch
```
Ich glaube, das war jetzt fast alles.
Dann sind da noch so bekloppte Fälle wie DEFAULT "ATTACKIERE hund"
Oder DEFAULT "NACH W"

Wenn du einen NMR hat, werden die jede Woche wiederholt.

Nein, ich glaube, das DEFAULT NACH erledigt sich nach einer Woche von selbst.
Der läuft einmal, und hat danach keinen langen Befehl mehr.
Jedenfalls soll man das nicht tun, und was da passiert, ist mir entsprechend auch fast völlig egal.

Ich meine Befehle:
```
ARBEITE
DEFAULT "NACH W"
```

Neue Vorlage:
```
NACH W
```

Im Falle eines NMR geht der nach Westen, und hat anschliessend keinen langen Befehl mehr (weil ein NACH wie ATTACKIERE nach Gebrauch verschwindet)
Im Falle keines NMR, wo du das NACH W einfach einsendest, macht er das, und setzt dann das vorherige NACH W als neuen Befehl.
Aber wie gesagt, alles akademisch, weil das niemand tun sollte.
Es gibt da keinen Anwendungsfall für.
Braucht man also auch in keine Anleitung zu schreiben, verwirrt nur.

## DEFAULT Befehl erklärt
DEFAULT heisst quasi "statt der langen Befehle, die ich diese Woche eingeschickt habe, möchte ich statt dessen diese hier in meine Vorlage übernehmen"
Wenn ich befehle:
```
LERNE Pferde
DEFAULT "MACHE Pferd"
DEFAULT "@GIB hein ALLES Pferd"
```
Dann sage ich "Diese Einheit soll diese Woche noch einmal Pferdedressur lernen, und danach für immer Pferde machen und weggeben"

Der Normalzustand einer Einheit ist, dass sie die gleichen Befehle immer wiederholt. Man braucht praktisch gesehen nur Befehle für sie einsenden, wenn man möchte, dass sie was neues tut.

Deswegen ist NACH eine Ausnahme. Niemand will, das eine Einheit für immer NACH WESTEN geht.
