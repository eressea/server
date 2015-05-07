##### convert extracted list to full list
sub strip {
    $_[0] =~ s/#(.*)#/\1/g

}

$, = "\t";		# set output field separator
$\ = "\n";		# set output record separator

$n = "<br />";              # in-field newline character

$textdelim = "#";       # text delimiter

$lastgebiet="none";
$lastletter="0";

$lettersorted=1;

while (<>){
    if (!($_ =~ /^#/)) {
    chomp;
    ($gebiet, $name, $beschreibung, $art, $stufe, $rang, $komponenten, $modifikationen, $syntax, $runde) = split($, , $_);


    strip($gebiet);
    if ($schoolsorted){
	if ("$lastgebiet" ne "$gebiet"){
	    print "";
	    print "== " . $gebiet . " ==";
	    print "";
	}
    }
    if ($lettersorted) {
	$letter = substr $name, 0, 1;
	if ("$lastletter" ne "$letter"){
	    print "";
	    print "== " . $letter . ".. ==";
	    print "";
	}
	$lastletter = $letter
    }
    
    $lastgebiet=$gebiet;

    strip($name);

    strip($beschreibung);
    $kurzbeschreibung = $beschreibung;
    $kurzbeschreibung =~ s/^([^.]*.).*$/\1/;

    strip($art);
    if ("$art" eq "Kampfzauber"){
	$kart="Kampf";
    }
    if ("$art" eq "Präkampfzauber"){
	$kart="K-Prä";
    }
    if ("$art" eq "Postkampfzauber"){
	$kart="K-Post";
    }
    if ("$art" eq "Normaler Zauber"){
	$kart="Normal";
    }

    strip($stufe);
    strip($rang);
    strip($komponenten);

    strip($modifikationen);
#    $modifikationen =~ s/Fernzauber/Fern/;
#    $modifikationen =~ s/Schiffszauber/Schiff/;
#    $modifikationen =~ s/Seezauber/See/;

    strip($syntax);
    $syntax =~ s/\\"/"/g;
    

    print "=== " . $name . " ===";
    print "";
    print "'''Beschreibung''':<br />$beschreibung<br />";
    print "'''Art''': $art<br />";
    print "'''Stufe''': $stufe<br />";
    print "'''Rang''': $rang<br />";
    print "'''Komponenten''': $komponenten<br />";
    print "'''Modifikationen''': $modifikationen<br />";
    print "'''Syntax''': $syntax<br /> ";
    print "";
    }
}

# von Hand: 
# Nebel der Verwirrung und Schleier der Verwirrung: 
# :!: Diesen Zauber gibt es nicht mehr. 
