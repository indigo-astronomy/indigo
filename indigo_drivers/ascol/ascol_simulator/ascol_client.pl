#!/usr/bin/perl
# Copyright (c) 2018 Rumen G. Bogdanovski
# All rights reserved.
#
# You can use this software under the terms of 'INDIGO Astronomy
# open-source license' (see LICENSE.md).
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# version history
# 0.1 by Rumen Bogdanovski <rumen@skyarchive.org>

# ASCOL telescope client console
# file ascol_client.pl

use strict;
use IO::Socket;
my ( $host, $port, $kidpid, $handle, $line, $timeout );
$host = '192.168.2.230';
$host = 'localhost';
$port = 2001;
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
