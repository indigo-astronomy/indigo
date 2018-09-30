#!/usr/bin/perl
use strict;
use IO::Socket;
my ( $host, $port, $kidpid, $handle, $line, $timeout );
$host = '192.168.1.230';
$host = 'localhost';
$port = 2000;
$timeout = 2;

$SIG{ALRM} = sub{ die "ALRM"; };
# create a tcp connection to the specified host and port
while(1) {
	print STDERR "Trying $host:$port...\n";
	eval {
		alarm($timeout); # set timeout for connect
		$handle = IO::Socket::INET->new(
			Proto    => "tcp",
			PeerAddr => $host,
			PeerPort => $port
		);
		alarm(0);
	};
	if ($handle) {last;}
	if ($port==2002) { print STDERR "Can't connect to $host.\n"; exit(1); }
	$port++;
}
$handle->autoflush(1);
 
print STDERR "[Connected to $host:$port]\n";
 
die "can't fork: $!" unless defined( $kidpid = fork() );
 
if ($kidpid) {
	while(defined($line = <$handle>)) {
		#$line=~s/\r//;
		#$line=~s/\f//;
		syswrite STDOUT, $line;
	}
	kill( "TERM", $kidpid );    # send SIGTERM to child
} else {
	while(defined($line = <STDIN>)) {
		print $handle "$line";
	}
}
