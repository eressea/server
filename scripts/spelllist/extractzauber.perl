#!/usr/bin/perl
eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
    if $running_under_some_shell;
			# this emulates #! processing on NIH machines.
			# (remove #! line above if indigestible)

eval '$'.$1.'$2;' while $ARGV[0] =~ /^([A-Za-z_0-9]+=)(.*)/ && shift;
			# process any FOO=bar switches

############ EXTRACTZAUBER
############ Author: stm
############ Datum 04/09
############ 
############ Filtert Zauberbeschreibung aus NRs nach CSV-Format
############
$, = "\t";		# set output field separator
$\ = "\n";		# set output record separator

$n = "\\\\";              # in-field newline character

$textdelim = "#";       # text delimiter
$interdelim = ", ";      # delimiter inside a record

$zauber = 0;

# Magiegebiet filtern

$runde = $ARGV[0];
$runde =~ s/^(.*[^0-9])*([0-9]+)-.*[.]nr/\2/;


while (<>) {
    chomp;	# strip record separator
    if (/.*\/(Gwyrrd|Illaun|Cerddor|Tybied|Draig|Gemein|Kein Magiegebiet)/) {
	$gebiet = $_;
	$gebiet =~ s/.*\/(Gwyrrd|Illaun|Cerddor|Tybied|Draig|Gemein|Kein Magiegebiet).*$/\1/g;
#	$gebiet = ($s = '.*/([^ ]*).*', s/$s/\1/g);
    }
    # Abschnitt Tränke beendet Abschnitt Zauber
    # Ausgabe des letzen Zaubers
    if ($zauber==1 && /Neue Tränke/) {
	if ($name ne '') {	#???
	    print $gebiet . $, . $name . $, . $textdelim . $beschreibung . $textdelim . $, . $art .

	      $, . $stufe . $, . $rang . $, . $textdelim . $komponenten . $textdelim . $, .

	      $modifikationen . $, . $textdelim . $syntax . $textdelim . $, . $runde;
	}
	$zauber = 0;
    }
    # Abschnitt Aktueller Status beendet Abschnitt Zauber
    # Ausgabe des letzen Zaubers
    if ($zauber==1 && /Aktueller Status/) {
	$zauber = 0;
    }
    if (zauber==0 && $_ =~ /.*Neue Zauber.*/) {
	$zauber = 1;
	print "#Gebiet" . $, . "Name" . $, . "Beschreibung" . $, . "Art" . $, . "Stufe" . $, . "Rang" . $, . "Komponenten" . $, . "Modifikationen" . $, . "Syntax" . $, . "Runde";
 	print "# Textdelimiter: #" . $, . "" . $, . "" . $, . "" . $, . "" . $, . "" . $, . "" . $, . "" . $, . "" . $, . "";
    }

    #Beginn eines Zaubers durch Name des Zaubers; beendet Syntaxmodus
    # Ausgabe letzter Zauber
    if ($zauber == 1 && $_ =~ /^      *.*$/ && !($_ =~ /  *Neue Zauber/)) {
	$syntaxstart = 0;
	if ($name ne '') {	#???
	    print $gebiet . $, . $name . $, . $textdelim . $beschreibung . $textdelim . $, . $art
		. $, . $stufe . $, . $rang . $, . $textdelim . $komponenten . $textdelim . $, .
		$modifikationen . $, . $textdelim . $syntax . $textdelim . $, . $runde;
	}
	$name = $_;
	$name =~ s/^ *([A-Z])/\1/g;
#	$name = ($s = '^ *([A-Z])', s/$s/\1/g);
    }

    # Art; beendet Beschreibungsmodus
    if ($zauber==1 && $beschreibungstart==1 && $_ =~ /^ *Art:.*$/) {
	$beschreibungstart = 0;
	$art = $_;
	$art =~ s/ *Art: ([A-Za-z ]*)/\1/;
    }

    # Wenn in Beschreibungsmodus: Anhägen an Beschreibung
    if ($zauber==1 && $beschreibungstart == 1) {
	$line = $_;
	$line =~ s/^ *//;
	# Escaping von Sonderzeichen
	$line =~ s/"/\\\"/g;
	if ($beschreibung eq '') {
	    $beschreibung = $line;
	}
	else {
	    $beschreibung = $beschreibung . ' ' . $line;
	}
    }
    # Start von Beschreibungsmodus
    if ($zauber == 1 && $_ =~ /^ *Beschreibung:.*$/) {
	$beschreibungstart = 1;
	$beschreibung = '';
    }

    # Stufe
    if ($zauber == 1 && $_ =~ /^ *Stufe:.*$/) {
	$stufe = $_;
	$stufe =~ s/ *Stufe: ([A-Za-z ]*)/\1/;
    }

    # Rang
    if ($zauber == 1 && $_ =~ /^ *Rang:.*$/) {
	$rang = $_;
	$rang =~ s/ *Rang: ([A-Za-z ]*)/\1/;
    }

    # Modifikationen; beendet Komponentenmodus
    if ($zauber==1 && $komponentenstart == 1 && $_ =~ /^ *Modifikationen:.*$/) {
	$komponentenstart = 0;
	$modifikationen = $_;
	$modifikationen =~ s/ *Modifikationen: *([A-Za-z ]*)/\1/;
    }

    # Komponente hinzufügen
    if ($zauber==1 && $komponentenstart == 1) {
	$komp = $_;
	$komp =~ s/^ *- *//;
	if ($komponenten eq '') {
	    $komponenten = $komp;
	}
	else {
	    $komponenten = $komponenten . $interdelim . $komp;
	}
    }
    # Start Komponentenmodus
    if ($zauber == 1 && $_ =~ /^ *Komponenten:.*$/) {
	$komponentenstart = 1;
	$komponenten = '';
    }

    # Syntaxzeile hinzufügen mit Escaping
    if ($zauber==1 && $syntaxstart == 1) {
	$line = $_;
	$line =~ s/"/\\\"/g;
	if ($line ne '') {	#???
	    if ($syntax eq '') {
		$syntax = $line;
	    }
	    else {
		$syntax = $syntax . $line;
	    }
	}
    }
    # Begin Syntaxmodus
    if ($zauber == 1 && $_ =~ /^ *Syntax:.*$/) {
	$syntaxstart = 1;
	$syntax = '';
    }

}
