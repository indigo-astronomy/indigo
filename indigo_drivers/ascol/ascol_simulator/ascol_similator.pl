#!/usr/bin/perl -w

use Time::HiRes qw ( setitimer ITIMER_VIRTUAL time );
use strict;
use IO::Socket;
use Net::hostent;

# OIL states
use constant OIL_OFF => 0;
use constant OIL_START1 => 1;
use constant OIL_START2 => 2;
use constant OIL_START3 => 3;
use constant OIL_ON => 4;
use constant OIL_OFF_DELAY => 5;

use constant OIL_START1_TIME => 5;
use constant OIL_START2_TIME => 10;
use constant OIL_START3_TIME => 15;
use constant OIL_OFF_DELAY_TIME => 5;

my $oil_state = OIL_OFF;
my $oil_time = 0;

# TELESCOPE states
use constant TE_INIT => 0;
use constant TE_OFF => 1;
use constant TE_OFF_WAIT => 2;
use constant TE_STOP => 3;
use constant TE_TRACK => 4;
use constant TE_OFF_REQ => 5;
use constant TE_SS_CLU1 => 6;
use constant TE_SS_SLEW => 7;
use constant TE_SS_DECC2 => 18;
use constant TE_SS_CLU2 => 8;
use constant TE_SS_DECC3 => 17;
use constant TE_SS_CLU3 => 9;
use constant TE_ST_DECC1 => 10;
use constant TE_ST_CLU1 => 11;
use constant TE_ST_SLEW => 12;
use constant TE_ST_DECC2 => 13;
use constant TE_ST_CLU2 => 14;
use constant TE_ST_DECC3 => 15;
use constant TE_ST_CLU3 => 16;

use constant TE_OFF_WAIT_TIME => 5;

use constant TE_CLU1_TIME => 2;
use constant TE_SLEW_TIME => 18;
use constant TE_DECC2_TIME => 20;
use constant TE_CLU2_TIME => 22;
use constant TE_DECC3_TIME => 24;
use constant TE_CLU3_TIME => 26;

my $te_state = TE_OFF;
my $te_rd_move_time = 0;
my $te_hd_move_time = 0;

my $correction_model = 0;
my $state_bits = 0;

my $guide_value_ra = 0;
my $guide_value_de = 0;
my $guide_correction_ra = 0;
my $guide_correction_de = 0;
my $user_speed_ra = 0;
my $user_speed_de = 0;
my $speed1 = 0;
my $speed2 = 0;
my $speed3 = 0;

my $de_centering_flag = 0;

my $client;

my $login = 1;
my $set_ra = 0;
my $set_de = 0;
my $req_ra = 0;
my $req_de = 0;
my $west = 0;
my $set_ha = 0;
my $req_ha = 0;

my $newrd = 0;
my $newhd = 0;


sub in_range($$$$) {
	my ($num, $min, $max, $accuracy) = @_;
	if (($num =~ /^[-+]?(\d+(\.\d{0,$accuracy})?)$/) and
	    ($num >= $min) and
		($num <= $max)) {
			return 1;
		}
	return 0;
}

sub is_ra($) {
	my ($ra) = @_;
	if ($ra =~ /^([0-2][0-3][0-5][0-9][0-5][0-9])(\.\d{0,2})?$/) {
		return 1;
	} elsif (($ra =~ /^(\d{1,3}(\.\d{0,6})?)$/) and ($ra > 0.0) and ($ra < 360.0)) {
			return 1;
	}
	return 0;
}

sub is_de($) {
	my ($de) = @_;
	if ($de =~ /^[-+]?([0-8][0-9][0-5][0-9][0-5][0-9])(\.\d{0,2})?$/) {
		return 1;
	} elsif (($de =~ /^[-+]?(\d{1,2}(\.\d{0,6})?)$/) and ($de > -90.0) and ($de < 90.0)) {
		return 1;
	}
	return 0;
}

sub dms2dd($$$) {
	my ($deg, $min, $sec) = @_;

	my $result = $deg + ($min/60) + (($sec/60) *(1/60));
	$result = sprintf("%.6f", $result);
}

sub dd2dms($) {
	my $input = @_;
	my $deg = floor($input);

	$input = $input - $deg;
	$input = $input * 60;

	my $min = floor($input);

	$input = $input - $min;
	$input = $input * 60;

	my $sec = sprintf("%.2f", $input);
	return ($deg, $min, $sec);
}


sub set_state {
	my $elapsed_time;

	# OIL state
	if ($oil_time != 0) {
		$elapsed_time = time() - $oil_time;
		if ($elapsed_time > OIL_START3_TIME) {
			$oil_state = OIL_ON;
			$oil_time = 0;
		} elsif ($elapsed_time > OIL_START2_TIME) {
			$oil_state = OIL_START3;
		} elsif ($elapsed_time > OIL_START1_TIME) {
			$oil_state = OIL_START2;
		}
	}

	# TELSECOPE state
	if ($te_rd_move_time != 0) {
		$elapsed_time = time() - $te_rd_move_time;
		if ($elapsed_time > TE_CLU3_TIME) {
			$set_ra=$req_ra;
			$set_de=$req_de;
			$te_state = TE_TRACK;
			$te_rd_move_time = 0;
		} elsif ($elapsed_time > TE_DECC3_TIME) {
			$te_state = TE_ST_CLU3;
		} elsif ($elapsed_time > TE_CLU2_TIME) {
			$te_state = TE_ST_DECC3;
		} elsif ($elapsed_time > TE_DECC2_TIME) {
			$te_state = TE_ST_CLU2;
		} elsif ($elapsed_time > TE_SLEW_TIME) {
			$te_state = TE_ST_DECC2;
		} elsif ($elapsed_time > TE_CLU1_TIME) {
			$te_state = TE_ST_SLEW;
		}
	}

	if ($te_hd_move_time != 0) {
		$elapsed_time = time() - $te_hd_move_time;
		if ($elapsed_time > TE_CLU3_TIME) {
			$set_ha=$req_ha;
			$set_de=$req_de;
			$te_state = TE_TRACK;
			$te_hd_move_time = 0;
		} elsif ($elapsed_time > TE_DECC3_TIME) {
			$te_state = TE_SS_CLU3;
		} elsif ($elapsed_time > TE_CLU2_TIME) {
			$te_state = TE_SS_DECC3;
		} elsif ($elapsed_time > TE_DECC2_TIME) {
			$te_state = TE_SS_CLU2;
		} elsif ($elapsed_time > TE_SLEW_TIME) {
			$te_state = TE_SS_DECC2;
		} elsif ($elapsed_time > TE_CLU1_TIME) {
			$te_state = TE_SS_SLEW;
		}
	}
}


 #you have to specify the port number after the perl command
 my $port=shift || die "Usage server.pl <port>\n";

 my $server = IO::Socket::INET->new(
 	Proto => 'tcp',
 	LocalPort => $port,
 	Listen => SOMAXCONN,
 	Reuse => 1
);

die "can't start simulator" unless $server;
print "[Server $0 is running]\n";

 ####
 #the server waits for a client to connect
 #and accept it with $server->accept
 #you can telnet to the port you want to test it
 ####

while ($client = $server->accept()) {
	$client->autoflush(1);
	my $hostinfo = gethostbyaddr($client->peeraddr);

 	#print this informations on the server side
  	printf "[Connect from %s]\n", $hostinfo->name || $client->peerhost;

 	while ( my $line = <$client>) {
		set_state();
		next unless $line=~/\S/; # blank line
		my @cmd = split /\s+/,$line;

		if ($cmd[0] eq "OION") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if (($te_state != 1) && ($oil_state != OIL_ON) && ($cmd[1] eq "0")) {
				print $client "ERR\n";
				next;
			}
			if ($cmd[1] eq "1") {
				$oil_state = OIL_START1;
				$oil_time = time();
				print $client "1\n";
				next;
			}
			if ($cmd[1] eq "0") {
				$oil_state=OIL_OFF;
				print $client "1\n";
				next;
			}
			print $client "ERR\n";
			next;
		}

		if ($cmd[0] eq "TEON") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if (($oil_state != OIL_ON) && ($cmd[1] == 1)) {
				print $client "ERR\n";
				next;
			}
			if (($oil_state == OIL_ON) && ($cmd[1] eq "1")) {
				$te_state = TE_STOP;
				print $client "1\n";
				next;
			}
			if (($oil_state == OIL_ON) && ($cmd[1] eq "0")) {
				$te_state = TE_OFF;
				print $client "1\n";
				next;
			}
			print $client "ERR\n";
			next;
		}

		if ($cmd[0] eq "TETR") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if (($te_state != TE_STOP) && ($cmd[1] == 1)) {
				print $client "ERR\n";
				next;
			}
			if (($te_state == TE_STOP) && ($cmd[1] eq "1")) {
				$te_state = TE_TRACK;
				print $client "1\n";
				next;
			}
			if (($oil_state == TE_TRACK) && ($cmd[1] eq "0")) {
				$te_state = TE_STOP;
				print $client "1\n";
				next;
			}
			print $client "ERR\n";
			next;
		}

		if ($cmd[0] eq "TSRA") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 3) { print $client "ERR\n"; next;}
			if ($te_state == TE_OFF) { print $client "ERR\n"; next;}
			if (!is_ra($cmd[1]) or !is_de($cmd[2])) {print $client "ERR\n"; next;}
			if(($cmd[3] ne "0") and ($cmd[3] ne "1")) { print $client "ERR\n"; next;};
			$req_ra=$cmd[1];
			$req_de=$cmd[2];
			$west=$cmd[3];
			$newrd=1;
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TSRR") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 2) { print $client "ERR\n"; next;}
			if ($te_state == TE_OFF) { print $client "ERR\n"; next;}
			$req_ra+=$cmd[1];
			$req_de+=$cmd[2];
			$newrd=1;
			print $client "1\n";
			next;
		}

		if (($cmd[0] eq "TGRA") or ($cmd[0] eq "TGRR")) {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state != TE_TRACK) { print $client "ERR\n"; next;}
			if($newrd) {
				$te_state = TE_ST_CLU1;
				$newrd = 0;
				$te_rd_move_time = time();
			}
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TSHA") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 2) { print $client "ERR\n"; next;}
			if ($te_state == TE_OFF) { print $client "ERR\n"; next;}
			$req_ha=$cmd[1];
			$req_de=$cmd[2];
			$newhd=1;
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TSHR") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 2) { print $client "ERR\n"; next;}
			if ($te_state == TE_OFF) { print $client "ERR\n"; next;}
			$req_ha+=$cmd[1];
			$req_de+=$cmd[2];
			$newhd=1;
			print $client "1\n";
			next;
		}

		if (($cmd[0] eq "TGHA") or ($cmd[0] eq "TGHR")) {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state != TE_TRACK) { print $client "ERR\n"; next;}
			if($newhd) {
				$te_state = TE_SS_CLU1;
				$newhd = 0;
				$te_hd_move_time = time();
			}
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TSCS") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state != TE_STOP) { print $client "ERR\n"; next;}
			if (($cmd[1] ne "0") and ($cmd[1] ne "1") and ($cmd[1] ne "2") and ($cmd[1] ne "3")) { print $client "ERR\n"; next;};
			$correction_model = $cmd[1];
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TSCA") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state != TE_STOP) { print $client "ERR\n"; next;}
			if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print $client "ERR\n"; next;};
			if ($cmd[1] == 1) {
				$state_bits = $state_bits | (1 << 4);
			} else {
				$state_bits = $state_bits & ~(1 << 4);
			}
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TSCP") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state != TE_STOP) { print $client "ERR\n"; next;}
			if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print $client "ERR\n"; next;};
			if ($cmd[1] == 1) {
				$state_bits = $state_bits | (1 << 5);
			} else {
				$state_bits = $state_bits & ~(1 << 5);
			}
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TSCR") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state != TE_STOP) { print $client "ERR\n"; next;}
			if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print $client "ERR\n"; next;};
			if ($cmd[1] == 1) {
				$state_bits = $state_bits | (1 << 6);
			} else {
				$state_bits = $state_bits & ~(1 << 6);
			}
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TSCM") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state != TE_STOP) { print $client "ERR\n"; next;}
			if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print $client "ERR\n"; next;};
			if ($cmd[1] == 1) {
				$state_bits = $state_bits | (1 << 7);
			} else {
				$state_bits = $state_bits & ~(1 << 7);
			}
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TSGM") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state != TE_STOP) { print $client "ERR\n"; next;}
			if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print $client "ERR\n"; next;};
			if ($cmd[1] == 1) {
				$state_bits = $state_bits | (1 << 8);
			} else {
				$state_bits = $state_bits & ~(1 << 8);
			}
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TRRD") {
			if ($#cmd!=0) { print $client "ERR\n"; next;}
			print $client "$set_ra $set_de $west\n";
			next;
		}

		if ($cmd[0] eq "TRHD") {
			if ($#cmd!=0) { print $client "ERR\n"; next;}
			print $client "$set_ha $set_de\n";
			next;
		}

		if ($cmd[0] eq "TSGV") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 2) { print $client "ERR\n"; next;}
			if ($te_state == TE_OFF) { print $client "ERR\n"; next;}
			if (in_range($cmd[1], -3600, 3600, 1) and in_range($cmd[2], -3600, 3600, 1)) {
				$guide_value_ra=$cmd[1];
				$guide_value_de=$cmd[2];
				print $client "1\n";
				next;
			}
		}

		if ($cmd[0] eq "TRGV") {
			if ($#cmd!=0) { print $client "ERR\n"; next;}
			print $client "$guide_value_ra $guide_value_de\n";
			next;
		}

		if ($cmd[0] eq "TSGC") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 2) { print $client "ERR\n"; next;}
			if ($te_state == TE_OFF) { print $client "ERR\n"; next;}
			if (in_range($cmd[1], -10, 10, 2) and in_range($cmd[2], 10, 10, 2)) {
				$guide_correction_ra = $cmd[1];
				$guide_correction_de = $cmd[2];
				print $client "1\n";
				next;
			}
		}

		if ($cmd[0] eq "TECE") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state != TE_STOP) { print $client "ERR\n"; next;}
			if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print $client "ERR\n"; next;};
			$de_centering_flag = $cmd[1];
			print $client "1\n";
			next;
		}

		if ($cmd[0] eq "TSUS") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 2) { print $client "ERR\n"; next;}
			if ($te_state == TE_OFF) { print $client "ERR\n"; next;}
			if (in_range($cmd[1], -10, 10, 4) and in_range($cmd[2], -10, 10, 4)) {
				$user_speed_ra = $cmd[1];
				$user_speed_de = $cmd[2];
				print $client "1\n";
				next;
			}
		}

		if ($cmd[0] eq "TRUS") {
			if ($#cmd!=0) { print $client "ERR\n"; next;}
			print $client "$user_speed_ra $user_speed_de\n";
			next;
		}

		if ($cmd[0] eq "TSS1") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state == TE_OFF) { print $client "ERR\n"; next;}
			if (in_range($cmd[1], 100, 5000, 2)) {
				$speed1 = $cmd[1];
				print $client "1\n";
				next;
			}
		}

		if ($cmd[0] eq "TRS1") {
			if ($#cmd!=0) { print $client "ERR\n"; next;}
			print $client "$speed1\n";
			next;
		}

		if ($cmd[0] eq "TSS2") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state == TE_OFF) { print $client "ERR\n"; next;}
			if (in_range($cmd[1], 1, 120, 2)) {
				$speed2 = $cmd[1];
				print $client "1\n";
				next;
			}
		}

		if ($cmd[0] eq "TRS2") {
			if ($#cmd!=0) { print $client "ERR\n"; next;}
			print $client "$speed2\n";
			next;
		}

		if ($cmd[0] eq "TSS3") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($te_state == TE_OFF) { print $client "ERR\n"; next;}
			if (in_range($cmd[1], 1, 120, 2)) {
				$speed3 = $cmd[1];
				print $client "1\n";
				next;
			}
		}

		if ($cmd[0] eq "TRS3") {
			if ($#cmd!=0) { print $client "ERR\n"; next;}
			print $client "$speed3\n";
			next;
		}

		if ($cmd[0] eq "GLST") {
			if ($#cmd!=0) { print $client "ERR\n"; next;}
			print $client "$oil_state $te_state 0 0 0 0 0 0 0 0 0 0 0 $correction_model $state_bits 0 0 0 0 0 0 0\n";
			next;
		}

		if ($line=~/quit|exit/i) {
			last;
  		} else {
      		print $client "ERR\n";
		}
	}
  	close $client;
}
