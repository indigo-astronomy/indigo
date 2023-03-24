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
	      "options:\n".
	      "       -v verbose output\n".
	      "       -h print help\n";
}

sub split_line ($){
	my ($line) = @_;
	my %fields;

	my @split1 = split (/\s+/, $line, 6);
	if (($split1[2] eq "B" || $split1[2] eq "0") && $split1[4] =~ /Define|Delete|Update/) {
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
		return %fields;
	} else {
		return undef;
	}
	#print "$fields{time} + $fields{app} + $fields{socket} + $fields{direction} + $fields{device} + $fields{property} + $fields{permission} + $fields{state}\n";
}

sub print_line (%$) {
	my (%fields, $verbose) = @_;
	my $act = "??";
	if ($fields{action} eq "Define") {
		$act = "++";
	} elsif ($fields{action} eq "Update") {
		$act = "==";
	} elsif ($fields{action} eq "Delete") {
		$act = "--";
	}
	my $message = "$fields{time} $act '$fields{device}'.'$fields{property}' $fields{action} -> $fields{state}\n";
	if (%fields{state} eq "Ok") {
		print GREEN $message;
	} elsif (%fields{state} eq "Busy"){
		print YELLOW $message;
	} elsif (%fields{state} eq "Alert"){
		print RED $message;
	} else {
		print GREY $message;
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

	foreach my $line (<STDIN>) {
		chomp($line);
		my %fields = split_line($line);
		if ($#pattern == 1) {
			if (%fields{device} eq $pattern[0] && %fields{property} eq $pattern[1]) {
				print_line(%fields, $verbose);
			}
		} elsif ($#pattern == 0) {
			if (%fields{device} eq $pattern[0]) {
				print_line(%fields, $verbose);
			}
		} elsif ($#pattern < 0) {
			print_line(%fields, $verbose);
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

	foreach my $line (<STDIN>) {
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
		$verbose && print GREY "match cmplete.\n";
		exit 0;

	} elsif ($command eq "trace") {
		if (!trace(@ARGV)) {
			$verbose && print RED "trace returned error.\n";
			exit 1;
		}
		$verbose && print GREY "trace cmplete.\n";
		exit 0;

	} else {
		print RED "There is no command \"$command\".\n";
	}
}

main;
