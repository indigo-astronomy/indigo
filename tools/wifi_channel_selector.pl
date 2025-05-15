#!/usr/bin/perl

# Copyright (c) 2021-2025 Rumen G. Bogdanovski <rumenastro@gmail.com>
# All rights reserved.
#
# You can use this software under the terms of 'INDIGO Astronomy
# open-source license'
# (see https://github.com/indigo-astronomy/indigo/blob/master/LICENSE.md).
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
# This script is called in indigo_server and rpi_ctrl.sh
# within the Raspbian GNU/Linux distribution to find the best
# WiFi channel for the access point.

use warnings;
use strict;
use Getopt::Long;

my $version = "0.1";
my $iwlist = '/usr/sbin/iwlist';
my $interface = "wlan0";
my $verbose;

sub index_min($) {
   my ($arr) = @_;
   return undef if !@$arr;

   my $idx = 0;
   for (1..$#$arr) { $idx = $_ if $arr->[$_] < $arr->[$idx]; }
   return $idx;
}

sub find_best_channel($) {
	my ($interface) = shift @_;

	my @signal = (0) x 15;
	my $channel;
	my $pid = open(CHLIST, "$iwlist $interface scan |") or return undef;
	while (my $line = <CHLIST>) {
		chomp($line);
		if ($line =~ /Channel:(\d+)/) {
			$verbose and print "$line -> $1\n";
			if ($1 > 13) {
				$channel = -1;
			} else {
				$channel = $1;
			}
		}
		if (($line =~ /Quality=(\d+)\/(\d+)/) and ($channel > 0)) {
			$verbose and print "$line -> $1 / $2\n";
			my $ch_signal = $1/$2;
			if ($channel > 1) {
				$signal[$channel-1] += $ch_signal / 3;
			}
			$signal[$channel] += $ch_signal;
			$signal[$channel + 1] += $ch_signal / 3;
		}
	}
	close(CHLIST);

	my @noise = (0) x 15;
	$noise[0] = 100;
	$noise[14] = 100;

	for (my $channel = 1; $channel < 14; $channel++) {
		$noise[$channel] += $signal[$channel-1] / 4 + $signal[$channel] + $signal[$channel+1] / 4;
		$verbose and printf "%3d -> %6.4f %6.4f\n", $channel, $signal[$channel], $noise[$channel];
	}

	return index_min(\@noise);
}

sub help() {
	print "usage: $0 [options]\n";
	print "\t--interface <interface> (default wlan0)\n";
	print "\t--verbose\n";
	print "\t--help\n";
	print "version: $version, written by Rumen G. Bogdanovski <rumen\@skyarchive.org>\n";
}

sub main() {
	GetOptions (
		"interface=s" => \$interface,    # numeric
		"verbose"  => \$verbose,
		"help" => sub { help(); exit 1; }
	) or die("Error in command line arguments\n");

	my $best_channel = find_best_channel($interface);
	if (!defined $best_channel) {
		warn "Can not determine best channel\n";
		exit 1;
	}
	print "$best_channel\n";
	exit 0;
}

main;
