#!/usr/bin/perl
###############################################################
# Copyright (c) 2023 Rumen G.Bogdanovski
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

# version history
# 0.1 by Rumen G.Bogdanovski <rumenastro@gmail.com>

###############################################################

use strict;
use Date::Parse;
use File::Basename;
use Getopt::Std;
use if $^O eq "MSWin32", "Win32::Console::ANSI";
use Term::ANSIColor qw(:constants);
$Term::ANSIColor::AUTORESET = 1;

my $VERSION = "0.1";

my $port;
my $verbose;

sub print_help() {
	my $N = basename($0);
	print "INDIGO Log Analyzer version $VERSION\n".
	      "usage: $N trace [options] [device1][.property1] [device2][.property2] ... < logfile\n".
		  "       $N match [options] pattern1 [pattern2] ... < logfile\n".
		  "       $N list_requests [options] [client1] [client2] ... < logfile\n".
		  "       $N list_clients [options] < logfile\n".
		  "       $N list_devices [options] < logfile\n".
	      "options:\n".
	      "       -v verbose output\n".
		  "       -w black and white output\n".
	      "       -h print help\n";
}

sub parse_record ($){
	my ($line) = @_;
	my %fields;

	my @split1 = split (/\s+/, $line, 6);
	if (($split1[2] eq "B" || $split1[2] eq "0") && !($split1[5] =~ /^-/)) {
		if ($split1[4] =~ /Define|Remove|Update|Change|Enable|Enumerate/) {
			$fields{time} = $split1[0];
			$fields{app} = $split1[1];
			$fields{socket} = $split1[2];
			$fields{direction} = $split1[3];
			$fields{action} = $split1[4];
			my @split2 = split (/\'/, $split1[5]);
			$fields{device} = $split2[1];
			$fields{property} = $split2[3];
			my @split3 = split (/\s+/, $split2[4]);
			$fields{type} = $split3[1];
			$fields{permission} = $split3[2];
			$fields{state} = $split3[3];

			$line =~ /.*\'(.*?)\'\s+\{.*/;
			if ($1 ne "") {
				$fields{client} = $1;
			}

			if ($fields{direction} =~ /\+/ || $line =~ /\{|[^\}]/) {
				while (my $l = <STDIN>) {
					if ($l =~ /\}/) {
						last;
					}
					if ($fields{type} eq "BLOB") {
						$l =~ /\'(.*?)\'\s+(.*)/;
						my $key = $1;
						my $value = $2;
						$fields{items}{$1} = $2;
					} else {
						my @kv = split(/\=/, $l);
						$kv[0] =~ /.*\'(.*?)\'.*/;
						my $key = $1;
						my $value;
						if ($kv[1] =~ /\'/) {
							$kv[1] =~ /.*\'(.*?)\'.*/;
							$value = $1;
						} else {
							$kv[1] =~ /^\s+(.*?)\s.*/;
							$value = $1;
						}
						$fields{items}{$key} = $value;
					}
				}
			}
			return %fields;
		}
	} else {
		return undef;
	}
}

sub print_line (%$) {
	my (%fields) = @_;
	my $act = "<>";
	if ($fields{action} eq "Define") {
		$act = "++";
	} elsif ($fields{action} eq "Update") {
		$act = "==";
	} elsif ($fields{action} eq "Remove") {
		$act = "--";
	} elsif ($fields{action} eq "Enable") {
		$act = "**";
	} elsif ($fields{action} eq "Enumerate") {
		$act = "##";
	}

	if ($verbose) {
		my $message = "$fields{time} $act $fields{action} -> $fields{state}";
		if (defined($fields{client})) {
			$message .= " ($fields{client})\n";
		} else {
			$message .= "\n";
		}

		my $message2 = "    '$fields{device}'.'$fields{property}'\n";

		if ($fields{action} eq "Enumerate") {
			print MAGENTA $message;
			print MAGENTA $message2;
			foreach my $key (keys %{$fields{items}}) {
				print MAGENTA "    $key = $fields{items}{$key}\n";
			}
		} elsif ($fields{action} eq "Enable") {
			print CYAN $message;
			print CYAN $message2;
			foreach my $key (keys %{$fields{items}}) {
				print CYAN "    $key = $fields{items}{$key}\n";
			}
		} elsif ($fields{state} eq "Ok") {
			print GREEN $message;
			print GREEN $message2;
			foreach my $key (keys %{$fields{items}}) {
				print GREEN "    $key = $fields{items}{$key}\n";
			}
		} elsif ($fields{state} eq "Busy"){
			print YELLOW $message;
			print YELLOW $message2;
			foreach my $key (keys %{$fields{items}}) {
				print YELLOW "    $key = $fields{items}{$key}\n";
			}
		} elsif ($fields{state} eq "Alert"){
			print RED $message;
			print RED $message2;
			foreach my $key (keys %{$fields{items}}) {
				print RED "    $key = $fields{items}{$key}\n";
			}
		} else {
			print WHITE $message;
			print WHITE $message2;
			foreach my $key (keys %{$fields{items}}) {
				print WHITE "    $key = $fields{items}{$key}\n";
			}
		}
		print "\n";
	} else {
		my $message = "$fields{time} $act '$fields{device}'.'$fields{property}' $fields{action} -> $fields{state}";
		if (defined($fields{client})) {
			$message .= " ($fields{client})\n";
		} else {
			$message .= "\n";
		}

		if ($fields{action} eq "Enumerate") {
			print MAGENTA $message;
		} elsif ($fields{action} eq "Enable") {
			print CYAN $message;
		} elsif ($fields{state} eq "Ok") {
			print GREEN $message;
		} elsif ($fields{state} eq "Busy"){
			print YELLOW $message;
		} elsif ($fields{state} eq "Alert"){
			print RED $message;
		} else {
			print WHITE $message;
		}
	}
}

sub trace_cmd {
	my @params = @_;

	while (my $line = <STDIN>) {
		chomp($line);

		my %fields = parse_record($line);
		if ($fields{time} eq "") {
			next;
		}

		if ($#params < 0) {
			print_line(%fields);
			next;
		}

		foreach my $property (@params) {
			my @pattern = split(/\./, $property);
			if ($#pattern == 1) {
				if ((%fields{device} eq $pattern[0] && %fields{property} eq $pattern[1]) || (%fields{device} eq "")) {
					print_line(%fields);
					last;
				}
			} elsif ($#pattern == 0) {
				if ((%fields{device} eq $pattern[0]) || (%fields{device} eq "")) {
					print_line(%fields);
					last;
				}
			}
		}
	}
	return 1;
}

sub list_requests_cmd {
	my @clients = @_;

	while (my $line = <STDIN>) {
		chomp($line);
		my %fields = parse_record($line);
		if ($#clients < 0 && $fields{action} eq "Change") {
			print_line(%fields);
		}
		foreach my $client (@clients) {
			if ($fields{client} eq $client) {
				print_line(%fields);
				last;
			}
		}
	}
	return 1;
}

sub list_devices_cmd {
	my @params = @_;

	if ($#params >= 0) {
		print RED "list_devices: Wrong parameters.\n";
		return undef;
	}

	while (my $line = <STDIN>) {
		chomp($line);

		my @split1 = split (/\s+/, $line, 6);
		my $time = $split1[0];

		$line =~ /.*\s+Attach\s+device\s+\'(.*?)\'$/;
		if ($1 ne "") {
			print GREEN "$time ++ '$1' attached\n";
			next;
		}

		$line =~ /.*\s+Detach\s+device\s+\'(.*?)\'$/;
		if ($1 ne "") {
			print RED "$time -- '$1' detached\n";
			next;
		}
	}
	return 1;
}

sub list_clients_cmd {
	my @params = @_;

	if ($#params >= 0) {
		print RED "list_clients: Wrong parameters.\n";
		return undef;
	}

	while (my $line = <STDIN>) {
		chomp($line);

		my @split1 = split (/\s+/, $line, 6);
		my $time = $split1[0];

		$line =~ /.*\s+Attach\s+client\s+\'(.*?)\'$/;
		if ($1 ne "") {
			print GREEN "$time ++ '$1' attached\n";
			next;
		}

		$line =~ /.*\s+Detach\s+client\s+\'(.*?)\'$/;
		if ($1 ne "") {
			print RED "$time -- '$1' detached\n";
			next;
		}
	}
	return 1;
}



sub match_cmd {
	my @paterns = @_;

	if ($#paterns < 0) {
		print RED "match: Wrong parameters.\n";
		return undef;
	}

	while (my $line = <STDIN>) {
		chomp($line);
		my $found = 1; 
		foreach my $patern (@paterns) {
			if ($line =~ /$patern/) {
			} else {
				$found = 0;
				last;
			}
		}
		$found && print "$line\n";
	}
	return 1;
}

sub main() {
	my %options = ();
	my $command = shift @ARGV;

	if (getopts("vhw", \%options) == undef) {
		exit 1;
	}

	if (defined($options{w})) {
		$ENV{ANSI_COLORS_DISABLED} = 1;
	}

	if (defined($options{h}) or !defined($command) or $command eq "-h") {
		print_help();
		exit 1;
	}

	if(defined($options{v})) {
		$verbose = 1;
	}

	if ($command eq "match") {
		if (!match_cmd(@ARGV)) {
			$verbose && print RED "match returned error\n";
			exit 1;
		}
		$verbose && print WHITE "match cmplete\n";
		exit 0;

	} elsif ($command eq "list_requests") {
		if (!list_requests_cmd(@ARGV)) {
			$verbose && print RED "list_requests returned error\n";
			exit 1;
		}
		$verbose && print WHITE "list_requests cmplete\n";
		exit 0;

	} elsif ($command eq "list_devices") {
		if (!list_devices_cmd(@ARGV)) {
			$verbose && print RED "list_devices returned error\n";
			exit 1;
		}
		$verbose && print WHITE "list_devices cmplete\n";
		exit 0;

	} elsif ($command eq "list_clients") {
		if (!list_clients_cmd(@ARGV)) {
			$verbose && print RED "list_clients returned error\n";
			exit 1;
		}
		$verbose && print WHITE "list_clients cmplete\n";
		exit 0;

	} elsif ($command eq "trace") {
		if (!trace_cmd(@ARGV)) {
			$verbose && print RED "trace returned error\n";
			exit 1;
		}
		$verbose && print WHITE "trace cmplete\n";
		exit 0;

	} else {
		print RED "There is no command \"$command\".\n";
	}
}

main;
