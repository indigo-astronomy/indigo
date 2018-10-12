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

# FOCUS states
use constant FO_STOP	=> 1;
use constant FO_MOVE	=> 4;
use constant FO_SET_POS => 5;

use constant FO_REL_MOVING_TIME => 5;
use constant FO_ABS_MOVING_TIME => 10;

# DOME states
use constant DO_OFF	   => 0;
use constant DO_STOP	   => 1;
use constant DO_PLUS	   => 2;
use constant DO_MINUS	   => 3;
use constant DO_SLEW_PLUS  => 4;
use constant DO_SLEW_MINUS => 5;
use constant DO_AUTO_STOP  => 6;
use constant DO_AUTO_PLUS  => 7;
use constant DO_AUTO_MINUS => 8;
use constant DO_SET_POS    => 9;
use constant DO_MOVE       => 10;

use constant DO_REL_MOVING_TIME => 5;
use constant DO_ABS_MOVING_TIME => 20;

# DOME SLIT states
use constant DO_SL_UNDEF   => 0;
use constant DO_SL_OPENING => 1;
use constant DO_SL_CLOSING => 2;
use constant DO_SL_OPEN	   => 3;
use constant DO_SL_CLOSE   => 4;

use constant DO_SL_OPENING_TIME => 10;
use constant DO_SL_CLOSING_TIME => 10;

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

my $fo_pos = 0;
my $fo_state = FO_STOP;
my $fo_rel_moving_time = 0;
my $fo_abs_moving_time = 0;

my $do_pos = 0;
my $do_state = DO_OFF;
my $do_rel_moving_time = 0;
my $do_abs_moving_time = 0;
my $do_sl_opening_time = 0;
my $do_sl_closing_time = 0;

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

sub dms2dd($) {
	my ($encoded) = @_;
	my $sign = 1;
	my $deg;
	my $min;
	my $sec;

	if ($encoded =~ /^[-+]/) {
		$deg = substr($encoded, 1, 2);
		$min = substr($encoded, 3, 2);
		$sec = substr($encoded, 5);
		$sign = -1 if ($encoded =~ /^-/);
	} else {
		$deg = substr($encoded, 0, 2);
		$min = substr($encoded, 2, 2);
		$sec = substr($encoded, 4);
	}

	my $result = ($deg + ($min/60) + (($sec/60) *(1/60))) * $sign;
	$result = sprintf("%.6f", $result);

	return $result;
}

sub dd2dms($) {
	my ($input) = @_;

	my $sign = 1;
	if ($input < 0) {
		$sign = -1;
		$input = -1 * $input;
	}
	my $deg = floor($input);

	$input = $input - $deg;
	$input = $input * 60;

	my $min = floor($input);

	$input = $input - $min;
	$input = $input * 60;

	my $sec = sprintf("%.2f", $input);
	my $result;
	if ($sign < 0) {
		$result = sprintf("-%02d%02d%05.2f", $deg, $min, $sec);
	} else {
		$result = sprintf("%02d%02d%05.2f", $deg, $min, $sec);
	}
	return $result;
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

	# FOCUS state
	if ($fo_rel_moving_time != 0) {
		$elapsed_time = time() - $fo_rel_moving_time;
		if ($elapsed_time > FO_REL_MOVING_TIME) {
			$fo_state = FO_STOP;
			$fo_rel_moving_time = 0;
		}
	}

	if ($fo_abs_moving_time != 0) {
		$elapsed_time = time() - $fo_abs_moving_time;
		if ($elapsed_time > FO_ABS_MOVING_TIME) {
			$fo_state = FO_STOP;
			$fo_abs_moving_time = 0;
		}
	}

	# DOME state
	if ($do_rel_moving_time != 0) {
		$elapsed_time = time() - $do_rel_moving_time;
		if ($elapsed_time > DO_REL_MOVING_TIME) {
			$do_state = DO_STOP;
			$do_rel_moving_time = 0;
		}
	}

	if ($do_abs_moving_time != 0) {
		$elapsed_time = time() - $do_abs_moving_time;
		if ($elapsed_time > DO_ABS_MOVING_TIME) {
			$do_state = DO_STOP;
			$do_abs_moving_time = 0;
		}
	}

	# DOME SLIT state
	if ($do_sl_opening_time != 0) {
		$elapsed_time = time() - $do_sl_opening_time;
		if ($elapsed_time > DO_SL_OPENING_TIME) {
			$do_state = DO_SL_OPEN;
			$do_sl_opening_time = 0;
		}
	}
	if ($do_sl_closing_time != 0) {
		$elapsed_time = time() - $do_sl_closing_time;
		if ($elapsed_time > DO_SL_CLOSING_TIME) {
			$do_state = DO_SL_CLOSE;
			$do_sl_closing_time = 0;
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
		#------------ Focus Position ---------- #
		if ($cmd[0] eq "FOPO") {
			if ($#cmd != 0) { print $client "ERR\n"; next;}
			print $client "$fo_pos\n";
			next;
		}
		# ------------- Focus Stop ------------ #
		if ($cmd[0] eq "FOST") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 0) { print $client "ERR\n"; next;}
			$fo_state = FO_STOP;
			print $client "1\n";
			next;
		}
		# ---- Focus Set Relative Position ---- #
		if ($cmd[0] eq "FOSR") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($fo_state != FO_STOP) { print $client "ERR\n"; next;}
			if (in_range($cmd[1], -49, 49, 0.01)) {
				$fo_pos = $cmd[1];
				$fo_state = FO_SET_POS;
				print $client "1\n";
				next;
			}
		}
		# ---- Focus Set Absolute Position ---- #
		if ($cmd[0] eq "FOSA") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($fo_state != FO_STOP) { print $client "ERR\n"; next;}
			if (in_range($cmd[1], 0, 49, 0.01)) {
				$fo_pos = $cmd[1];
				$fo_state = FO_SET_POS;
				print $client "1\n";
				next;
			}
		}
		# ---------- Focus Go Relative -------- #
		if ($cmd[0] eq "FOGR") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 0) { print $client "ERR\n"; next;}
			if ($fo_state != FO_SET_POS) { print $client "ERR\n"; next;}
			$fo_state = FO_MOVE;
			$fo_rel_moving_time = time();
			print $client "1\n";
			next;
		}
		# ---------- Focus Go Absolute -------- #
		if ($cmd[0] eq "FOGA") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 0) { print $client "ERR\n"; next;}
			if ($fo_state != FO_SET_POS) { print $client "ERR\n"; next;}
			$fo_state = FO_MOVE;
			$fo_abs_moving_time = time();
			print $client "1\n";
			next;
		}
		# -------------- Dome On -------------- #
		if ($cmd[0] eq "DOON") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if (($do_state == DO_OFF) && ($cmd[1] eq "1")) {
				$do_state = DO_SL_CLOSE;
				print $client "1\n";
				next;
			}
			if (($do_state == DO_SL_CLOSE) && ($cmd[1] eq "0")) {
				$do_state = DO_OFF;
				print $client "1\n";
				next;
			}
			print $client "ERR\n";
			next;
		}
		# ----------- Dome Slit Open ---------- #
		if ($cmd[0] eq "DOSO") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if (($do_state == DO_SL_CLOSE) && ($cmd[1] eq "1")) {
				$do_state = DO_SL_OPENING;
				$do_sl_opening_time = time();
				print $client "1\n";
				next;
			}
			if (($do_state == DO_SL_OPEN) && ($cmd[1] eq "0")) {
				$do_state = DO_SL_CLOSING;
				$do_sl_closing_time = time();
				print $client "1\n";
				next;
			}
			print $client "ERR\n";
			next;
		}
		# ------------ Dome Position ---------- #
		if ($cmd[0] eq "DOPO") {
			if ($#cmd != 0) { print $client "ERR\n"; next;}
			if ($do_state != DO_OFF) { print $client "$do_pos\n"; next; }
			print $client "ERR\n";
			next;
		}
		# ----- Dome Set Relative Position ---- #
		if ($cmd[0] eq "DOSR") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($do_state != DO_STOP) { print $client "ERR\n"; next;}
			if (in_range($cmd[1], -179.99, 180.00, 0.01)) {
				$do_pos = $cmd[1];
				$do_state = DO_SET_POS;
				print $client "1\n";
				next;
			}
		}
		# ----- Dome Set Absolute Position ---- #
		if ($cmd[0] eq "DOSA") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 1) { print $client "ERR\n"; next;}
			if ($do_state != DO_STOP) { print $client "ERR\n"; next;}
			if (in_range($cmd[1], 0.00, 359.99, 0.01)) {
				$do_pos = $cmd[1];
				$do_state = DO_SET_POS;
				print $client "1\n";
				next;
			}
		}
		#------ Dome Go Relative Position ----- #
		if ($cmd[0] eq "DOGR") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 0) { print $client "ERR\n"; next;}
			if ($do_state != DO_SET_POS) { print $client "ERR\n"; next;}
			$do_state = DO_MOVE;
			$do_rel_moving_time = time();
			print $client "1\n";
			next;
		}
		#------ Dome Go Absolute Position ----- #
		if ($cmd[0] eq "DOGA") {
			if (!$login) { print $client "ERR\n"; next;}
			if ($#cmd != 0) { print $client "ERR\n"; next;}
			if ($do_state != DO_SET_POS) { print $client "ERR\n"; next;}
			$do_state = DO_MOVE;
			$do_abs_moving_time = time();
			print $client "1\n";
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
