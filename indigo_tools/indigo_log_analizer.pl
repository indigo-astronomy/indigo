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
	      "usage: $N trace [options] [device][.property] < logfile\n".
		  "       $N match [options] pattern1 [pattern2] ... < logfile\n".
		  "       $N requests [options] [client1] [client2] ... < logfile\n".
	      "options:\n".
	      "       -v verbose output\n".
	      "       -h print help\n";
}

sub split_line ($){
	my ($line) = @_;
	my %fields;

	my @split1 = split (/\s+/, $line, 6);
	if (($split1[2] eq "B" || $split1[2] eq "0") && $split1[4] =~ /Define|Remove|Update|Change/ && !($split1[5] =~ /^-/)) {
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

		if ($fields{direction} =~ /\+/) {
			while (my $l = <STDIN>) {
				if ($l =~ /\}/) {
					last;
				}
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
		return %fields;
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
	}

	if ($verbose) {
		my $message = "$fields{time} $act $fields{action} -> $fields{state}";
		if (defined($fields{client})) {
			$message .= " ($fields{client})\n";
		} else {
			$message .= "\n";
		}

		my $message2 = "    '$fields{device}'.'$fields{property}'\n";

		if ($fields{state} eq "Ok") {
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
		if ($fields{state} eq "Ok") {
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

sub trace {
	my @params = @_;
	my $property;
	my @pattern;

	if ($#params == 0) {
		$property = $params[0];
		@pattern = split(/\./, $property);
	} else {
		print RED "trace: Wrong parameters.\n";
		return undef;
	}

	while (my $line = <STDIN>) {
		chomp($line);
		my %fields = split_line($line);
		if ($#pattern == 1) {
			if (%fields{device} eq $pattern[0] && %fields{property} eq $pattern[1]) {
				print_line(%fields);
			}
		} elsif ($#pattern == 0) {
			if (%fields{device} eq $pattern[0]) {
				print_line(%fields);
			}
		} elsif ($#pattern < 0) {
			print_line(%fields);
		}
	}
	return 1;
}

sub requests {
	my @clients = @_;

	while (my $line = <STDIN>) {
		chomp($line);
		my %fields = split_line($line);
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

sub match {
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

	if (getopts("vh", \%options) == undef) {
		exit 1;
	}

	if (defined($options{h}) or !defined($command) or $command eq "-h") {
		print_help();
		exit 1;
	}

	if(defined($options{v})) {
		$verbose = 1;
	}

	if ($command eq "match") {
		if (!match(@ARGV)) {
			$verbose && print RED "trace returned error.\n";
			exit 1;
		}
		$verbose && print WHITE "match cmplete.\n";
		exit 0;

	} elsif ($command eq "requests") {
		if (!requests(@ARGV)) {
			$verbose && print RED "requests returned error.\n";
			exit 1;
		}
		$verbose && print WHITE "requests cmplete.\n";
		exit 0;

	} elsif ($command eq "trace") {
		if (!trace(@ARGV)) {
			$verbose && print RED "trace returned error.\n";
			exit 1;
		}
		$verbose && print WHITE "trace cmplete.\n";
		exit 0;

	} else {
		print RED "There is no command \"$command\".\n";
	}
}

main;
