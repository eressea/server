#!/usr/bin/perl

use Getopt::Std;

sub output_file {
	my $file = shift(@_);

	foreach $dir (@searchpath) {
		$absname = $dir."/".$file;
		if ( -r $absname ) {
			system "cat $absname";
			last;
		}
	}
}

getopt('I');
@searchpath = split(/:/,$opt_I);

while(<STDIN>) {
	$line = $_;

	if( $line =~ /^\s*#include\s+\"(.*)\"/ ) {
		$file = $1;
		if(not ($file =~ /\.\./ or $file =~ /^\//) or $file =~ /;/) {
			output_file($file);
		}
	} else if ( $line =~ /^\s*#perl\s+\"(.*)\"/ ) {
		$script = $1;
		do $script;
	} else {
		print $line;
	}
}

