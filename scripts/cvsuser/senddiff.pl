#!/usr/bin/perl

require Mail::Send;

if(scalar(@ARGV) == 0) {
	exit();
}

$msg = new Mail::Send;
$msg->to('eressea-diff@eressea.upb.de');

# system('cvs update');

$no = shift(@ARGV);
$dir = shift(@ARGV);
($project, $dir) = split(/\//, $dir, 2);

$msg->set('Subject', "[commit $no] diff $project/$dir");
$mail = $msg->open();

print $mail "updated files in $project/$dir\n";
chdir('/home/cvs/checkout/eressea/$project/$dir');

foreach $arg (@ARGV) {
	($file, $oldver, $newver) = split(/,/, $arg);
	print $mail "COMMAND: cvs diff -u -r $oldver -r $newver $file\n\n";
	print $mail `cd /home/cvs/checkout/eressea/$project/$dir ; cvs diff -u -r $oldver -r $newver $file`;
}

$mail->close();
