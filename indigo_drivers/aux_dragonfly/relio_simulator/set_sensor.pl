#!/usr/bin/perl
use IO::Socket;
use strict;

my $sock = IO::Socket::INET->new(
    Proto    => 'udp',
    PeerPort => 10000,
    PeerAddr => 'localhost',
) or die "Could not create socket: $!\n";

$sock->send("!relio snset 0 $ARGV[0] $ARGV[1]#") or die "Send error: $!\n";
