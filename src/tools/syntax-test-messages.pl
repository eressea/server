#!/usr/bin/perl
# vi:set ts=2:

$errorcount = 0;
$linecount = 0;

while(<>) {
	$line = $_;

	$linecount++;

	if($line !~ /^\s*#/ && $line !~ /^\s*$/ && $line !~ /^\w+;(events|magic|errors|study|economy|battle|movement|production):[012345];(de|en);.+/) {
		print "syntax error in line $linecount: $line\n";
		$errorcount++;
	}
}

if($errorcount > 0) {
	print "$errorcount errors found.\n";
} else {
	print "No errors found.\n";
}

