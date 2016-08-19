##### convert extracted list to table (dokuwiki syntax)
sub strip {
    $_[0] =~ s/#(.*)#/\1/g

}

$, = "\t";		# set output field separator
#$\ = "\n";		# set output record separator

$n = "<br />";              # in-field newline character

$textdelim = "#";       # text delimiter

$lastgebiet="none";

$separate = 0;

my %spells = ();

%Gebiete = ('Cerddor' => 'C', 'Draig' =>  'D', 'Gwyrrd' =>  'G', 'Illaun' => 'I', 'Tybied' => 'T',
	    'Gemein' => 'A', 'Kein Magiegebiet' => 'N');

$suffix ="_E3";

while (<>) {
    chomp;
    if (!($_ =~ /^#/)) {
    ($gebiet, $name, $beschreibung, $art, $stufe, $rang, $komponenten, $modifikationen, $syntax, $runde) = split($, , $_);

    strip($gebiet);
    $gebiet = $Gebiete{$gebiet};

    strip($name);

    strip($beschreibung);
    $kurzbeschreibung = $beschreibung;
    $kurzbeschreibung =~ s/^([^.]*.).*$/\1/;

    strip($art);
    if ("$art" eq "Kampfzauber") {
	$kart="Kampf";
    }
    if ("$art" eq "Präkampfzauber") {
	$kart="K-Prä";
    }
    if ("$art" eq "Postkampfzauber") {
	$kart="K-Post";
    }
    if ("$art" eq "Normaler Zauber") {
	$kart="Normal";
    }

    strip($stufe);
    strip($rang);
    strip($komponenten);

    strip($modifikationen);
    $modifikationen =~ s/Fernzauber/Fern/;
    $modifikationen =~ s/Schiffszauber/Schiff/;
    $modifikationen =~ s/Seezauber/See/;
    $modifikationen =~ s/Kann nicht vom Vertrauten gezaubert werden/<sup>V<\/sup>/;

    strip($syntax);
    $syntax =~ s/\\"/"/g;

    if (not exists $spells{$name}) {
	$spells{$name} = {};
	foreach my $v (values %Gebiete) {
	    $spells{$name}{$v} = 'N';
	}
    }
    
    $spells{$name}{$gebiet} = 'J';
    $spells{$name}{'name'} = $name;
    $spells{$name}{'beschreibung'} = $kurzbeschreibung;
    $spells{$name}{'art'} = $kart;
    $spells{$name}{'stufe'} = $stufe;
    $spells{$name}{'rang'} = $rang;
    $spells{$name}{'komponenten'} = $komponenten;
    $spells{$name}{'modifikationen'} = $modifikationen;
    $spells{$name}{'syntax'} = $syntax;
    $spells{$name}{'runde'} = $runde;
    }
}

print "== Alle Zauber ==\n\n";
print "{| class=\"wikitable sortable\"\n";
print "! ";
foreach my $v (values %Gebiete) {
    print "$v !! ";
}
print "Name !! Kurzbeschreibung !! Art !! Stufe !! Rang !! Komponenten !! Modifikationen\n";


foreach my $name (sort { $spells{$a}{'name'} cmp $spells{$b}{'name'} } keys %spells ) {
    
    $name = $spells{$name}{'name'};
    $beschreibung = $spells{$name}{'beschreibung'};
    $art = $spells{$name}{'art'};
    $stufe = $spells{$name}{'stufe'};
    $rang = $spells{$name}{'rang'};
    $komponenten = $spells{$name}{'komponenten'};
    $modifikationen = $spells{$name}{'modifikationen'};
    $syntax = $spells{$name}{'syntax'};
    $runde = $spells{$name}{'runde'};

    
    print "|-\n|  ";
    foreach my $v (values %Gebiete) {
	print "$spells{$name}{$v} || ";
    }
    print "[[Zauberbeschreibungen$suffix#$name|$name]] || $beschreibung || $art || $stufe || $rang || $komponenten || $modifikationen\n";
}

$\ = "\n";		# set output record separator

print "|}";
foreach my $k (keys %Gebiete) {
    print "<sup>$Gebiete{$k}</sup> $k<br />";
}
print "<sup>V</sup> Kann nicht vom Vertrauten gezaubert werden<br />";
print "";
