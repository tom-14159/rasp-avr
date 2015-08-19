#!/usr/bin/perl
use strict;
use Term::ReadKey;
use Device::SerialPort;
use Time::HiRes qw(usleep nanosleep);

my $port = new Device::SerialPort("/dev/ttyAMA0");
#$port->user_msg(ON);
$port->baudrate(9600);
$port->parity("none");
$port->databits(8);
$port->stopbits(1);
$port->handshake("xoff");
$port->write_settings;

ReadMode 3;

my %cmds = (
	'w'	=> ["m1 1", "m2 1"],
	's'	=> ["m1 -1", "m2 -1"],
	'a'	=> ["m1 1", "m2 -1"],
	'd'	=> ["m2 1", "m1 -1"],
	'o'	=> ["l+"],
	'p'	=> ["l-"],
	'default' => ["m"],
);

my $key;
while ($key ne 'q') {
	while (not defined ($key = ReadKey(-1))) {}

	if ($cmds{ $key }) {
		print "$key\n";
		send_cmd($cmds{ $key });
	} else {
		send_cmd($cmds{"default"});
	}
}

$port->write("m\n");
print "bye\n";

ReadMode 0;

sub send_cmd {
	my $cmds = shift;

	for my $cmd (@$cmds) {
		$port->write("$cmd\r\n");
		usleep(50000);
	}
}
