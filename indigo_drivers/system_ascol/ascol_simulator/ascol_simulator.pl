#!/usr/bin/perl -w

# Copyright (c) 2018 Rumen G. Bogdanovski and Thomas Stibor
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
#        Thomas Stibor <thomas@stibor.net>

# ASCOL telescope simulator
# file ascol_simulator.pl

use strict;
use IO::Socket;
use Net::hostent;
use POSIX qw(ceil floor strftime);
use Time::HiRes qw(gettimeofday);
use Getopt::Std;
use threads;
use threads::shared;

my $verbose = 0;
my $login = 0;

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

# True if the telescope is tracking
# (no matter slewing ot not at the moment)
my $te_tracking = 0;

my $te_state = TE_OFF;
my $te_rd_abs_move_time = 0;
my $te_hd_abs_move_time = 0;
my $te_rd_rel_move_time = 0;
my $te_hd_rel_move_time = 0;

# Hour Axis states
use constant HA_STOP => 0;
use constant HA_POSITION => 1;
use constant HA_CA_CLU1 => 2;
use constant HA_CA_FAST => 3;
use constant HA_CA_FASTBR => 4;
use constant HA_CA_CLU2 => 5;
use constant HA_CA_SLOW => 6;
# more states ....

my $ha_move_time = 0;
my $ha_state = HA_STOP;

# Declination Axis states
use constant DA_STOP => 0;
use constant DA_POSITION => 1;
use constant DA_CA_CLU1 => 2;
use constant DA_CA_FAST => 3;
use constant DA_CA_FASTBR => 4;
use constant DA_CA_CLU2 => 5;
use constant DA_CA_SLOW => 6;
# more states ...

use constant CA_CLU1_TIME => 2;
use constant CA_FAST_TIME => 14;
use constant CA_FASTBR_TIME => 16;
use constant CA_CLU2_TIME => 18;
use constant CA_SLOW_TIME => 28;

my $da_move_time = 0;
my $da_state = HA_STOP;

# FOCUS states
use constant FO_OFF	=> 0;
use constant FO_STOP	=> 1;
use constant FO_PLUS	=> 2;
use constant FO_MINUS	=> 3;
use constant FO_SLEW	=> 4;

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

use constant DO_REL_MOVING_TIME => 5;
use constant DO_ABS_MOVING_TIME => 20;
use constant DO_AUTO_MOVING_TIME => 10;
use constant DO_AUTO_STOP_TIME => 60;

# SLIT states
use constant SL_UNDEF   => 0;
use constant SL_OPENING => 1;
use constant SL_CLOSING => 2;
use constant SL_OPEN	=> 3;
use constant SL_CLOSE   => 4;

use constant SL_OPENING_TIME => 10;
use constant SL_CLOSING_TIME => 10;

# FLAP TUBE states
use constant FL_TB_UNDEF   => 0;
use constant FL_TB_OPENING => 1;
use constant FL_TB_CLOSING => 2;
use constant FL_TB_OPEN    => 3;
use constant FL_TB_CLOSE   => 4;

use constant FL_TB_OPENING_TIME => 5;
use constant FL_TB_CLOSING_TIME => 5;

# FLAP COUDE states
use constant FL_CD_UNDEF   => 0;
use constant FL_CD_OPENING => 1;
use constant FL_CD_CLOSING => 2;
use constant FL_CD_OPEN    => 3;
use constant FL_CD_CLOSE   => 4;

use constant FL_CD_OPENING_TIME => 5;
use constant FL_CD_CLOSING_TIME => 5;

# Guide value set time
use constant GUIDE_VALUE_TIME => 0;

my $correction_model = 0;
my $state_bits : shared = 3; # HA and DA calibrated
my @alarm_bits : shared = (0,0,0,0,0);

my $req_guide_value_ra = 0;
my $req_guide_value_de = 0;
my $guide_value_ra = 0;
my $guide_value_de = 0;
my $guide_value_time = 0;

my $guide_correction_ra = 0;
my $guide_correction_de = 0;
my $user_speed_ra = 0;
my $user_speed_de = 0;
my $speed1 = 5000.00;
my $speed2 = 120.0;
my $speed3 = 10.00;

my $de_centering_flag = 0;

my $client;

my $set_ra = 0;
my $set_de = 0;

my $req_abs_ra = 0;
my $req_abs_de = 0;
my $req_rel_ra = 0;
my $req_rel_de = 0;

my $west = 0;
my $set_ha = 0;

my $req_abs_ha = 0;
my $req_rel_ha = 0;

my $new_abs_rd = 0;
#my $new_rel_rd = 0;
my $new_abs_hd = 0;
#my $new_rel_hd = 0;

my $req_abs_fo_pos = 0;
my $req_rel_fo_pos = 0;
my $fo_pos = 0;
my $fo_state = FO_OFF;
my $fo_rel_moving_time = 0;
my $fo_abs_moving_time = 0;

my $do_pos = 0;
my $do_pos_cur = 0;
my $do_state = DO_OFF;
my $do_rel_moving_time = 0;
my $do_abs_moving_time = 0;
my $do_auto_moving_time = 0;

my $sl_state = SL_CLOSE;
my $sl_opening_time = 0;
my $sl_closing_time = 0;

my $fl_tb_state = FL_TB_CLOSE;
my $fl_tb_opening_time = 0;
my $fl_tb_closing_time = 0;

my $fl_cd_state = FL_TB_CLOSE;
my $fl_cd_opening_time = 0;
my $fl_cd_closing_time = 0;

sub in_range($$$$) {
	my ($num, $min, $max, $accuracy) = @_;
	if (($num =~ /^[-+]?(\d+(\.\d{0,$accuracy})?)$/) and
	    ($num >= $min) and
	    ($num <= $max)) {
			return 1;
		}
	return 0;
}

# This function returns GMT as LST
# it is good enough for simulation ;)
sub fake_LST(){
	my ($s,$us) = gettimeofday();
	my $hours = (int($s % 86400) + $us / 1e6) / 3600.0;
	return $hours;
}

sub can_slew() {
	if ((($state_bits & 3) == 3) # H axis and Dec axis are calibrated
	   and (($alarm_bits[2] & 2) == 0)) # Bridge is parked
	{
		$verbose && print "Can Slew\n";
		return 1;
	}
	$verbose && print "Can NOT Slew\n";
	return 0;
}

sub bridge_parked() {
	if (($alarm_bits[2] & 2) == 0) {
		$verbose && print "Bridge is parked\n";
		return 1;
	}
	$verbose && print "Bridge is NOT parked\n";
	return 0;
}

sub parse_ra($) {
	my ($ra) = @_;
	if ($ra =~ /^([0-2][0-9][0-5][0-9][0-5][0-9])(\.\d{0,3})?$/) {
		return dms2dd($ra, 1);
	} elsif (($ra =~ /^(\d{1,3}(\.\d{0,6})?)$/) and ($ra >= 0.0) and ($ra < 360.0)) {
		return $ra;
	}
	return undef;
}

sub parse_de($) {
	my ($de) = @_;
	if ($de =~ /^[-+]?([0-8][0-9][0-5][0-9][0-5][0-9])(\.\d{0,2})?$/) {
		return dms2dd($de, 0);
	} elsif (($de =~ /^[-+]?(\d{1,2}(\.\d{0,6})?)$/) and ($de > -90.0) and ($de < 90.0)) {
		return $de;
	}
	return undef;
}

sub dms2dd($$) {
	my ($encoded, $hours) = @_;
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
	$result *= 15.0 if ($hours);
	$result = sprintf("%.6f", $result);

	return $result;
}

sub dd2dms($$) {
	my ($input, $hours) = @_;
	$input /= 15.0 if ($hours);
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

	my $sec = $input;

	if ($sec >= 59.9995) {
		$min++;
		$sec = 0;
	}
	if ($min >= 60) {
		$hours++;
		$min = 0;
	}

	my $result;
	if ($hours) {
		$result = sprintf("%02d%02d%06.3f", $deg, $min, $sec);
	} else {
		if ($sign < 0) {
			$result = sprintf("-%02d%02d%05.2f", $deg, $min, $sec);
		} else {
			$result = sprintf("+%02d%02d%05.2f", $deg, $min, $sec);
		}
	}
	return $result;
}

sub update_state {
	my $elapsed_time;

	#update HA or RA deepending on telesope state
	if ($te_tracking) {
		$set_ha = fake_LST()*15 - $set_ra;
		if ($set_ha < 0) {
			$set_ha += 360;
		}
	} else {
		$set_ra = fake_LST()*15 - $set_ha;
		if ($set_ra < 0) {
			$set_ra += 360;
		}
	}

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

	if ($guide_value_time != 0) {
		$elapsed_time = time() - $guide_value_time;
		if ($elapsed_time > GUIDE_VALUE_TIME) {
			$guide_value_ra = $req_guide_value_ra;
			$guide_value_de = $req_guide_value_de;
			$guide_value_time = 0;
		}
	}

	if ($te_rd_abs_move_time != 0) {
		$elapsed_time = time() - $te_rd_abs_move_time;
		if ($elapsed_time > TE_CLU3_TIME) {
			$set_ra=$req_abs_ra;
			$set_de=$req_abs_de;
			$te_state = TE_TRACK;
			$te_rd_abs_move_time = 0;
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

	if ($te_rd_rel_move_time != 0) {
		$elapsed_time = time() - $te_rd_rel_move_time;
		if ($elapsed_time > TE_CLU3_TIME) {
			$set_ra += $req_rel_ra/3600.0;
			$set_de += $req_rel_de/3600.0;
			$te_state = TE_TRACK;
			$te_rd_rel_move_time = 0;
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

	if ($te_hd_abs_move_time != 0) {
		$elapsed_time = time() - $te_hd_abs_move_time;
		if ($elapsed_time > TE_CLU3_TIME) {
			$set_ha = $req_abs_ha;
			$set_de = $req_abs_de;
			$te_state = TE_STOP;
			$te_hd_abs_move_time = 0;
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

	if ($te_hd_rel_move_time != 0) {
		$elapsed_time = time() - $te_hd_rel_move_time;
		if ($elapsed_time > TE_CLU3_TIME) {
			$set_ha += $req_rel_ha/3600.0;
			$set_de += $req_rel_de/3600.0;
			$te_state = TE_STOP;
			$te_hd_rel_move_time = 0;
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

	# Hour Axis Callibration
	if ($ha_move_time != 0) {
		$elapsed_time = time() - $ha_move_time;
		if ($elapsed_time > CA_SLOW_TIME) {
			# set cllibrated bit
			$state_bits = $state_bits | 1;
			$ha_state = HA_POSITION;
			$ha_move_time = 0;
		} elsif ($elapsed_time > CA_CLU2_TIME) {
			$ha_state = HA_CA_SLOW;
		} elsif ($elapsed_time > CA_FASTBR_TIME) {
			$ha_state = HA_CA_CLU2;
		} elsif ($elapsed_time > CA_FAST_TIME) {
			$ha_state = HA_CA_FASTBR;
		} elsif ($elapsed_time > CA_CLU1_TIME) {
			$ha_state = HA_CA_FAST;
		}
	}

	# Declination Axis Callibration
	if ($da_move_time != 0) {
		$elapsed_time = time() - $da_move_time;
		if ($elapsed_time > CA_SLOW_TIME) {
			# set cllibrated bit
			$state_bits = $state_bits | 2;
			$da_state = DA_POSITION;
			$da_move_time = 0;
		} elsif ($elapsed_time > CA_CLU2_TIME) {
			$da_state = DA_CA_SLOW;
		} elsif ($elapsed_time > CA_FASTBR_TIME) {
			$da_state = DA_CA_CLU2;
		} elsif ($elapsed_time > CA_FAST_TIME) {
			$da_state = DA_CA_FASTBR;
		} elsif ($elapsed_time > CA_CLU1_TIME) {
			$da_state = DA_CA_FAST;
		}
	}

	# FOCUS state
	if ($fo_rel_moving_time != 0) {
		$elapsed_time = time() - $fo_rel_moving_time;
		if ($elapsed_time > FO_REL_MOVING_TIME) {
			$fo_state = FO_STOP;
			$fo_pos += $req_rel_fo_pos;
			$fo_pos = 100 if ($fo_pos > 100);
			$fo_pos = 0 if ($fo_pos < 0);
			$fo_rel_moving_time = 0;
		}
	}

	if ($fo_abs_moving_time != 0) {
		$elapsed_time = time() - $fo_abs_moving_time;
		if ($elapsed_time > FO_ABS_MOVING_TIME) {
			$fo_state = FO_STOP;
			$fo_pos = $req_abs_fo_pos;
			$fo_abs_moving_time = 0;
		}
	}

	# DOME state
	if ($do_rel_moving_time != 0) {
		$elapsed_time = time() - $do_rel_moving_time;
		if (($do_state == DO_STOP) or ($do_pos == $do_pos_cur)) {
			$elapsed_time += DO_REL_MOVING_TIME + 1;
		} elsif ($do_state == DO_SLEW_MINUS) {
			$do_pos_cur = ($do_pos_cur - 1) % 360;
		} elsif ($do_state == DO_SLEW_PLUS) {
			$do_pos_cur = ($do_pos_cur + 1) % 360;
		}
		if ($elapsed_time > DO_REL_MOVING_TIME) {
			# When moving time is passed we magically jump to desired postion.
			if ($do_state != DO_STOP) {
				$do_pos_cur = $do_pos;
			}
			$do_state = DO_STOP;
			$do_rel_moving_time = 0;
		}
	}
	if ($do_abs_moving_time != 0) {
		$elapsed_time = time() - $do_abs_moving_time;
		if (($do_state == DO_STOP) or ($do_pos == $do_pos_cur)) {
			$elapsed_time += DO_ABS_MOVING_TIME + 1;
		} elsif ($do_state == DO_SLEW_MINUS) {
			$do_pos_cur = ($do_pos_cur - 1) % 360;
		} elsif ($do_state == DO_SLEW_PLUS) {
			$do_pos_cur = ($do_pos_cur + 1) % 360;
		}
		if ($elapsed_time > DO_ABS_MOVING_TIME) {
			# When moving time is passed we magically jump to desired postion.
			if ($do_state != DO_STOP) {
				$do_pos_cur = $do_pos;
			}
			$do_state = DO_STOP;
			$do_abs_moving_time = 0;
		}
	}
	if ($do_auto_moving_time != 0) {
		$elapsed_time = time() - $do_auto_moving_time;
		if ($do_state == DO_STOP) {
			$do_auto_moving_time = 0;
		} elsif (($elapsed_time > DO_AUTO_STOP_TIME) and ($do_state == DO_AUTO_STOP)) {
			if ($west) {
				$do_state = DO_AUTO_PLUS;
			} else {
				$do_state = DO_AUTO_MINUS;
			}
			$do_auto_moving_time = time();
		} elsif ($elapsed_time > DO_AUTO_MOVING_TIME) {
			$do_state = DO_AUTO_STOP;
		}
	}

	# SLIT state
	if ($sl_opening_time != 0) {
		$elapsed_time = time() - $sl_opening_time;
		if ($elapsed_time > SL_OPENING_TIME) {
			$sl_state = SL_OPEN;
			$sl_opening_time = 0;
		}
	}
	if ($sl_closing_time != 0) {
		$elapsed_time = time() - $sl_closing_time;
		if ($elapsed_time > SL_CLOSING_TIME) {
			$sl_state = SL_CLOSE;
			$sl_closing_time = 0;
		}
	}

	# FLAP TUBE state
	if ($fl_tb_opening_time != 0) {
		$elapsed_time = time() - $fl_tb_opening_time;
		if ($elapsed_time > FL_TB_OPENING_TIME) {
			$fl_tb_state = FL_TB_OPEN;
			$fo_state = FO_STOP;
			$fl_tb_opening_time = 0;
		}
	}
	if ($fl_tb_closing_time != 0) {
		$elapsed_time = time() - $fl_tb_closing_time;
		if ($elapsed_time > FL_TB_CLOSING_TIME) {
			$fl_tb_state = FL_TB_CLOSE;
			$fo_state = FO_OFF;
			$fl_tb_closing_time = 0;
		}
	}

	# FLAP COUDE state
	if ($fl_cd_opening_time != 0) {
		$elapsed_time = time() - $fl_cd_opening_time;
		if ($elapsed_time > FL_CD_OPENING_TIME) {
			$fl_cd_state = FL_CD_OPEN;
			$fl_cd_opening_time = 0;
		}
	}
	if ($fl_cd_closing_time != 0) {
		$elapsed_time = time() - $fl_cd_closing_time;
		if ($elapsed_time > FL_CD_CLOSING_TIME) {
			$fl_cd_state = FL_CD_CLOSE;
			$fl_cd_closing_time = 0;
		}
	}
}

sub print_client($$) {
	my ($client, $message) = @_;
	print $client $message;
	$verbose && print "Login: $login State: $oil_state $te_state $ha_state $da_state $fo_state 0 $do_state $sl_state $fl_tb_state $fl_cd_state 0 0 0 0 $correction_model $state_bits [@alarm_bits] 0\n";
}

# simulator console
sub console() {
	threads->detach();
	print "[ASCOL Simulator console. For help: \"help\" or \"?\" ]\n";
	while ( my $line = <STDIN>) {
		unless ($line=~/\S/) { next; }; # blank line
		my @cmd = split /\s+/,$line;

		if ($cmd[0] eq "set_state") {
			if ($#cmd != 1) { print "error\n"; next; }
			if (!in_range($cmd[1], 0, 15, 0)) { print "error\n"; next; }
			$state_bits = $state_bits | (1 << $cmd[1]);
			print "State bits: $state_bits\n";
			next;
		}
		if ($cmd[0] eq "clear_state") {
			if ($#cmd != 1) { print "error\n"; next; }
			if (!in_range($cmd[1], 0, 15, 0)) { print "error\n"; next; }
			$state_bits = $state_bits & ~(1 << $cmd[1]);
			print "State bits: $state_bits\n";
			next;
		}
		if ($cmd[0] eq "set_alarm") {
			if ($#cmd != 2) { print "error\n"; next; }
			if (!in_range($cmd[1], 0, 4, 0)) { print "error\n"; next; }
			if (!in_range($cmd[2], 0, 15, 0)) { print "error\n"; next; }
			$alarm_bits[$cmd[1]] = $alarm_bits[$cmd[1]] | (1 << $cmd[2]);
			print "Alarm bits: @alarm_bits\n";
			next;
		}
		if ($cmd[0] eq "clear_alarm") {
			if ($#cmd != 2) { print "error\n"; next; }
			if (!in_range($cmd[1], 0, 4, 0)) { print "error\n"; next; }
			if (!in_range($cmd[2], 0, 15, 0)) { print "error\n"; next; }
			$alarm_bits[$cmd[1]] = $alarm_bits[$cmd[1]] & ~(1 << $cmd[2]);
			print "Alarm bits: @alarm_bits\n";
			next;
		}
		if ($cmd[0] eq "park_bridge") {
			if ($#cmd != 0) { print "error\n"; next; }
			$alarm_bits[2] = $alarm_bits[2] & ~(1 << 1);
			print "Bridge parked: @alarm_bits\n";
			next;
		}
		if ($cmd[0] eq "unpark_bridge") {
			if ($#cmd != 0) { print "error\n"; next; }
			$alarm_bits[2] = $alarm_bits[2] | (1 << 1);
			print "Bridge unparked: @alarm_bits\n";
			next;
		}
		if ($cmd[0] eq "status") {
			if ($#cmd != 0) { print "error\n"; next; }
			print "Login: $login State: $oil_state $te_state $ha_state $da_state $fo_state 0 $do_state $sl_state $fl_tb_state $fl_cd_state 0 0 0 0 $correction_model $state_bits [@alarm_bits] 0\n";
			next;
		}
		if (($cmd[0] eq "help") or ($cmd[0] eq "?")) {
			print "Valid console commands:\n";
			print "   status                   :print status\n";
			print "   set_state <bit>          :set state bit (0-15)\n";
			print "   clear_state <bit>        :clear state bit (0-15)\n";
			print "   set_alarm <bank> <bit>   :set alarm bit (0-15) of bank (0-4)\n";
			print "   clear_alarm <bank> <bit> :clear alarm bit (0-15) of bank (0-4)\n";
			print "   park_bridge              :alias for \"set_alarm 2 1\"\n";
			print "   unpark_bridge            :alias for \"clear_alarm 2 1\"\n";
			print "   exit                     :terminate simulator\n";
			print "   help or ?                :this help\n";
			next;
		}
		if ($cmd[0] eq "exit") {
			print "Bye!\n";
			exit(0);
		}
		print "error\n";
	}
}

sub print_usage() {
	print "\nASCOL Telescope Simulator\n";
	print "usage: $0 [-lotTvh] [-p port]\n\n";
	print "    -l        : Do not require GLLG\n";
	print "    -o        : Oil is ON at startup (OION 1)\n";
	print "    -t        : Telescope is ON at startup (TEON 1)\n";
	print "    -T        : Telescope tracking is ON at startup (TETR 1)\n";
	print "    -v        : Print global status on each command\n";
	print "    -h        : Show this help\n";
	print "    -p port   : TCP port to listen on (default: 2000)\n";
	print "\n    example: $0 -l -t -p 2001\n\n";
	exit;
}

sub main() {
	my %opt=();
	getopts("lotTvhp:", \%opt) or print_usage();

	if (defined $opt{h}) {
		print_usage();
	}
	my $port = 2000;
	if (defined $opt{p}) {
		$port = $opt{p};
	}
	if (defined $opt{l}) {
		print "[Login required: OFF]\n";
		# $login is set to 1 at connect
	}
	if (defined $opt{o}) {
		$oil_state = OIL_ON;
		print "[Oil: ON]\n";
	}
	if (defined $opt{t}) {
		$oil_state = OIL_ON;
		print "[Oil: ON]\n";
		$te_state = TE_STOP;
		$te_tracking = 0;
		$da_state = DA_POSITION;
		$ha_state = HA_POSITION;
		print "[Telescope: ON (Tracking: OFF)]\n";
	}
	if (defined $opt{T}) {
		$oil_state = OIL_ON;
		print "[Oil: ON]\n";
		$te_state = TE_TRACK;
		$te_tracking = 1;
		$da_state = DA_POSITION;
		$ha_state = HA_POSITION;
		print "[Telescope: ON (Tracking: ON)]\n";
	}
	if (defined $opt{v}) {
		$verbose = 1;
	}

	my $server = IO::Socket::INET->new(
		Proto => 'tcp',
		LocalPort => $port,
		Listen => SOMAXCONN,
		Reuse => 1
	);

	die "[Can not start simulator on port: $port]" unless $server;
	print "[ASCOL Simulator is running on port: $port]\n";

	# console thread is detatched
	threads->create(\&console);

	while ($client = $server->accept()) {
		if (defined $opt{l}) { $login = 1; }

		$client->autoflush(1);
		my $hostinfo = gethostbyaddr($client->peeraddr);

		#print this informations on the server side
		printf "[Accepted connection from: %s]\n", $hostinfo->name || $client->peerhost;

		while ( my $line = <$client>) {
			update_state();
			unless ($line=~/\S/) { print_client($client, "ERR\n"); next; }; # blank line
			my @cmd = split /\s+/,$line;

			# ----- Oil pump start/stop ----- #
			if ($cmd[0] eq "OION") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if ($te_state != TE_OFF) {
					print_client($client, "1\n");
					next;
				}
				if ($cmd[1] eq "1") {
					if ($oil_state == OIL_OFF) {
						$oil_state = OIL_START1;
						$oil_time = time();
					}
					print_client($client, "1\n");
					next;
				}
				if ($cmd[1] eq "0") {
					$oil_state=OIL_OFF;
					print_client($client, "1\n");
					next;
				}
				print_client($client, "ERR\n");
				next;
			}

			# ----- Oil Sensors Readings ----- #
			if ($cmd[0] eq "OIMV") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				# Hardcoded but these do not change rapidly
				if ($oil_state == OIL_ON) {
					print_client($client, "70.3 71.5 24.8 25.0 21.7 26.4 25.3 28.1 21.7 20.9 27.5 23.1 72.1 88.8 49.0 17.7 46.0\n");
				} elsif ($oil_state == OIL_OFF) {
					print_client($client, "0.0 0.4 0.0 0.2 0.0 0.1 0.1 0.3 0.0 0.0 0.2 0.2 32.5 88.5 69.0 13.6 12.8\n");
				} else {
					print_client($client, "11.6 12.6 11.9 12.3 11.9 12.4 12.2 12.4 11.5 11.5 12.6 12.5 31.2 88.7 44.0 18.0 48.3\n");
				}
				next;
			}

			# ----- Telescope ON/OFF ----- #
			if ($cmd[0] eq "TEON") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($oil_state != OIL_ON) && ($cmd[1] == 1)) {
					print_client($client, "1\n");
					next;
				}
				if (($oil_state == OIL_ON) && ($cmd[1] eq "1")) {
					if ($te_state == TE_OFF) {
						$te_state = TE_STOP;
						$te_tracking = 0;
						$da_state = DA_POSITION;
						$ha_state = HA_POSITION;
					}
					print_client($client, "1\n");
					next;
				}
				if ($cmd[1] eq "0") {
					$te_state = TE_OFF;
					$te_tracking = 0;
					$da_state = DA_STOP;
					$ha_state = HA_STOP;
					print_client($client, "1\n");
					next;
				}
				print_client($client, "ERR\n");
				next;
			}

			if ($cmd[0] eq "TETR") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($te_state != TE_STOP) && ($cmd[1] == 1)) {
					print_client($client, "1\n");
					next;
				}
				if (($te_state == TE_STOP) && ($cmd[1] eq "1")) {
					$te_state = TE_TRACK;
					$te_tracking = 1;
					print_client($client, "1\n");
					next;
				}
				if (($oil_state == TE_TRACK) && ($cmd[1] eq "0")) {
					$te_state = TE_STOP;
					$te_tracking = 0;
					print_client($client, "1\n");
					next;
				}
				print_client($client, "ERR\n");
				next;
			}

			if ($cmd[0] eq "TSRA") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 3) { print_client($client, "ERR\n"); next; }
				if ($te_state == TE_OFF) { print_client($client, "1\n"); next; }
				my $ra = parse_ra($cmd[1]);
				my $de = parse_de($cmd[2]);
				if (!defined($ra) or !defined($de)) {print_client($client, "ERR\n"); next; }
				if(($cmd[3] ne "0") and ($cmd[3] ne "1")) { print_client($client, "ERR\n"); next; };
				$req_abs_ra = $ra;
				$req_abs_de = $de;
				$west = $cmd[3];
				$new_abs_rd = 1;
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TSRR") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 2) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], -36000, 36000, 2) and in_range($cmd[2], -36000, 36000, 2)) {
					if ($te_state == TE_OFF) { print_client($client, "1\n"); next; }
					$req_rel_ra = $cmd[1];
					$req_rel_de = $cmd[2];
					#$new_rel_rd = 1;
					print_client($client, "1\n");
					next;
				}
			}

			if ($cmd[0] eq "TGRA") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ((($te_state != TE_TRACK) and ($cmd[1] == 1)) or !can_slew() or !$te_tracking) {
					print_client($client, "1\n");
					next;
				}
				if ($cmd[1] == 1) {
					if($new_abs_rd) {
						$te_state = TE_ST_CLU1;
						$new_abs_rd = 0;
						#$new_rel_rd = 1;
						$te_rd_abs_move_time = time();
					}
				} else {
					# simplyfy stop -> should go to state transition
					$te_state = TE_TRACK;
					$new_abs_rd = 1;
					$te_rd_abs_move_time = 0;
				}
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TGRR") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ((($te_state != TE_TRACK) and ($cmd[1] == 1)) or !$te_tracking) {
					print_client($client, "1\n");
					next;
				}
				if ($cmd[1] == 1) {
					#if($new_rel_rd) {
						$te_state = TE_ST_CLU1;
						#$new_rel_rd = 0;
						$new_abs_rd = 1;
						$te_rd_rel_move_time = time();
					#}
				} else {
					# simplyfy stop -> should go to state transition
					$te_state = TE_TRACK;
					#$new_rel_rd = 1;
					$te_rd_rel_move_time = 0;
				}
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TSHA") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 2) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], -180, 330, 4) and in_range($cmd[2], -90, 270, 4)) {
					if ($te_state == TE_OFF) { print_client($client, "1\n"); next; }
					$req_abs_ha = $cmd[1];
					$req_abs_de = $cmd[2];
					$new_abs_hd = 1;
					print_client($client, "1\n");
					next;
				}
			}

			if ($cmd[0] eq "TSHR") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 2) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], -36000, 36000, 2) and in_range($cmd[2], -36000, 36000, 2)) {
					if ($te_state == TE_OFF) { print_client($client, "1\n"); next; }
					$req_rel_ha = $cmd[1];
					$req_rel_de = $cmd[2];
					#$new_rel_hd = 1;
					print_client($client, "1\n");
					next;
				}
			}

			# ----- GOTO HA and DEC ----- #
			if ($cmd[0] eq "TGHA") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ((($te_state != TE_STOP) and ($cmd[1] == 1)) or !can_slew() or $te_tracking) {
					print_client($client, "1\n");
					next;
				}
				if ($cmd[1] == 1) {
					if($new_abs_hd) {
						$te_state = TE_SS_CLU1;
						$new_abs_hd = 0;
						#$new_rel_hd = 1;
						$te_hd_abs_move_time = time();
					}
				} else {
					# simplyfy stop -> shuld go to state transition
					$te_state = TE_STOP;
					$new_abs_hd = 1;
					$te_hd_abs_move_time = 0;
				}
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TGHR") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ((($te_state != TE_STOP) and ($cmd[1] == 1)) or $te_tracking) {
					print_client($client, "1\n");
					next;
				}
				if ($cmd[1] == 1) {
					#if($new_rel_hd) {
						$te_state = TE_SS_CLU1;
						#$new_rel_hd = 0;
						$new_abs_hd = 1;
						$te_hd_rel_move_time = time();
					#}
				} else {
					# simplyfy stop -> shuld go to state transition
					$te_state = TE_STOP;
					#$new_rel_hd = 1;
					$te_hd_rel_move_time = 0;
				}
				print_client($client, "1\n");
				next;
			}

			# ----- Callibrate Hour Axis ----- #
			if ($cmd[0] eq "TEHC") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ((($te_state != TE_STOP) and ($cmd[1] == 1)) or !bridge_parked()) { print_client($client, "1\n"); next; }
				if ($cmd[1] == 1) {
					$ha_state = HA_CA_CLU1;
					$ha_move_time = time();
				} else {
					# simplyfy stop -> shuld go to state transition
					$ha_state = HA_POSITION;
					$ha_move_time = 0;
				}
				print_client($client, "1\n");
				next;
			}

			# ----- Callibrate Declination Axis ----- #
			if ($cmd[0] eq "TEDC") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ((($te_state != TE_STOP) and ($cmd[1] == 1)) or !bridge_parked()) { print_client($client, "1\n"); next; }
				if ($cmd[1] == 1) {
					$da_state = DA_CA_CLU1;
					$da_move_time = time();
				} else {
					# simplyfy stop -> shuld go to state transition
					$da_state = DA_POSITION;
					$da_move_time = 0;
				}
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TSCS") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1") and ($cmd[1] ne "2") and ($cmd[1] ne "3") and ($cmd[1] ne "4")) { print_client($client, "ERR\n"); next; };
				if ($te_state != TE_STOP) { print_client($client, "1\n"); next; }
				$correction_model = $cmd[1];
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TSCA") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ($te_state != TE_STOP) { print_client($client, "1\n"); next; }
				if ($cmd[1] == 1) {
					$state_bits = $state_bits | (1 << 4);
				} else {
					$state_bits = $state_bits & ~(1 << 4);
				}
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TSCP") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ($te_state != TE_STOP) { print_client($client, "1\n"); next; }
				if ($cmd[1] == 1) {
					$state_bits = $state_bits | (1 << 5);
				} else {
					$state_bits = $state_bits & ~(1 << 5);
				}
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TSCR") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ($te_state != TE_STOP) { print_client($client, "1\n"); next; }
				if ($cmd[1] == 1) {
					$state_bits = $state_bits | (1 << 6);
				} else {
					$state_bits = $state_bits & ~(1 << 6);
				}
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TSCM") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ($te_state != TE_STOP) { print_client($client, "1\n"); next; }
				if ($cmd[1] == 1) {
					$state_bits = $state_bits | (1 << 7);
				} else {
					$state_bits = $state_bits & ~(1 << 7);
				}
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TSGM") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ($te_state != TE_STOP) { print_client($client, "1\n"); next; }
				if ($cmd[1] == 1) {
					$state_bits = $state_bits | (1 << 8);
				} else {
					$state_bits = $state_bits & ~(1 << 8);
				}
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TRRD") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				my $ra = dd2dms($set_ra, 1);
				my $de = dd2dms($set_de, 0);
				print_client($client, "$ra $de $west\n");
				next;
			}

			if ($cmd[0] eq "TRHD") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				my $resp = sprintf "%.4f %.4f\n", $set_ha - 180, $set_de;
				print_client($client, $resp);
				next;
			}

			if ($cmd[0] eq "TRRA") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				my $ra = dd2dms($req_abs_ra, 1);
				my $de = dd2dms($req_abs_de, 0);
				print_client($client, "$ra $de $west\n");
				next;
			}

			if ($cmd[0] eq "TRHA") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				my $resp = sprintf "%.4f %.4f\n", $req_abs_ha - 180, $req_abs_de;
				print_client($client, $resp);
				next;
			}

			if ($cmd[0] eq "TRRR") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				my $resp = sprintf "%.2f %.2f\n", $req_rel_ra, $req_rel_de;
				print_client($client, $resp);
				next;
			}

			if ($cmd[0] eq "TRHR") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				my $resp = sprintf "%.2f %.2f\n", $req_rel_ha, $req_rel_de;
				print_client($client, $resp);
				next;
			}

			if ($cmd[0] eq "TSGV") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 2) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], -3600, 3600, 1) and in_range($cmd[2], -3600, 3600, 1)) {
					if ($te_state == TE_OFF) { print_client($client, "1\n"); next; }
					$req_guide_value_ra = $cmd[1];
					$req_guide_value_de = $cmd[2];
					$guide_value_time = time();
					print_client($client, "1\n");
					next;
				}
			}

			if ($cmd[0] eq "TRGV") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				print_client($client, "$guide_value_ra $guide_value_de\n");
				next;
			}

			if ($cmd[0] eq "TSGC") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 2) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], -10, 10, 2) and in_range($cmd[2], -10, 10, 2)) {
					if ($te_state == TE_OFF) { print_client($client, "1\n"); next; }
					$guide_correction_ra = $cmd[1];
					$guide_correction_de = $cmd[2];
					print_client($client, "1\n");
					next;
				}
			}

			if ($cmd[0] eq "TECE") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if ($te_state != TE_STOP) { print_client($client, "1\n"); next; }
				$de_centering_flag = $cmd[1];
				print_client($client, "1\n");
				next;
			}

			if ($cmd[0] eq "TSUS") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 2) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], -10, 10, 4) and in_range($cmd[2], -10, 10, 4)) {
					if ($te_state == TE_OFF) { print_client($client, "1\n"); next; }
					$user_speed_ra = $cmd[1];
					$user_speed_de = $cmd[2];
					print_client($client, "1\n");
					next;
				}
			}

			if ($cmd[0] eq "TRUS") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				print_client($client, "$user_speed_ra $user_speed_de\n");
				next;
			}

			if ($cmd[0] eq "TSS1") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], 100, 5000, 2)) {
					if ($te_state == TE_OFF) { print_client($client, "1\n"); next; }
					$speed1 = $cmd[1];
					print_client($client, "1\n");
					next;
				}
			}

			if ($cmd[0] eq "TRS1") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				print_client($client, "$speed1\n");
				next;
			}

			if ($cmd[0] eq "TSS2") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], 1, 120, 2)) {
					if ($te_state == TE_OFF) { print_client($client, "1\n"); next; }
					$speed2 = $cmd[1];
					print_client($client, "1\n");
					next;
				}
			}

			if ($cmd[0] eq "TRS2") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				print_client($client, "$speed2\n");
				next;
			}

			if ($cmd[0] eq "TSS3") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], 1, 120, 2)) {
					if ($te_state == TE_OFF) { print_client($client, "1\n"); next; }
					$speed3 = $cmd[1];
					print_client($client, "1\n");
					next;
				}
			}

			if ($cmd[0] eq "TRS3") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				print_client($client, "$speed3\n");
				next;
			}
			#------------ Focus Position ---------- #
			if ($cmd[0] eq "FOPO") {
				if ($#cmd != 0) { print_client($client, "ERR\n"); next; }
				print_client($client, "$fo_pos\n");
				next;
			}
			# ------------- Focus Stop ------------ #
			if ($cmd[0] eq "FOST") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 0) { print_client($client, "ERR\n"); next; }
				if ($fo_state == FO_OFF) { print_client($client, "1\n"); next; }
				$fo_state = FO_STOP;
				$fo_rel_moving_time = 0;
				$fo_abs_moving_time = 0;
				print_client($client, "1\n");
				next;
			}
			# ---- Focus Set Relative Position ---- #
			if ($cmd[0] eq "FOSR") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], -49, 49, 2)) {
					if ($fo_state == FO_OFF) { print_client($client, "1\n"); next; }
					$req_rel_fo_pos = $cmd[1];
					print_client($client, "1\n");
					next;
				}
			}
			# ---- Focus Set Absolute Position ---- #
			if ($cmd[0] eq "FOSA") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], 0, 49, 2)) {
					if ($fo_state == FO_OFF) { print_client($client, "1\n"); next; }
					$req_abs_fo_pos = $cmd[1];
					print_client($client, "1\n");
					next;
				}
			}
			# ---------- Focus Go Relative -------- #
			if ($cmd[0] eq "FOGR") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 0) { print_client($client, "ERR\n"); next; }
				if ($fo_state == FO_OFF) { print_client($client, "1\n"); next; }
				$fo_state = FO_SLEW;
				$fo_rel_moving_time = time();
				print_client($client, "1\n");
				next;
			}
			# ---------- Focus Go Absolute -------- #
			if ($cmd[0] eq "FOGA") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 0) { print_client($client, "ERR\n"); next; }
				if ($fo_state == FO_OFF) { print_client($client, "1\n"); next; }
				$fo_state = FO_SLEW;
				$fo_abs_moving_time = time();
				print_client($client, "1\n");
				next;
			}
			# -------------- Dome On -------------- #
			if ($cmd[0] eq "DOON") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($cmd[1] ne "0") and ($cmd[1] ne "1")) { print_client($client, "ERR\n"); next; }
				if (($do_state == DO_OFF) && ($cmd[1] eq "1")) {
					$do_state = DO_STOP;
				}
				if (($do_state == DO_STOP) and ($sl_state == SL_CLOSE)
				    && ($cmd[1] eq "0")) {
					$do_state = DO_OFF;
				}
				print_client($client, "1\n");
				next;
			}
			# ----------- Dome Slit Open ---------- #
			if ($cmd[0] eq "DOSO") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($sl_state == SL_CLOSE) and ($do_state == DO_STOP) && ($cmd[1] eq "1")) {
					$sl_state = SL_OPENING;
					$sl_opening_time = time();
					print_client($client, "1\n");
					next;
				}
				if (($sl_state == SL_OPEN) && ($cmd[1] eq "0")) {
					$sl_state = SL_CLOSING;
					$sl_closing_time = time();
					print_client($client, "1\n");
					next;
				}
				print_client($client, "ERR\n");
				next;
			}
			# ------------ Dome Position ---------- #
			if ($cmd[0] eq "DOPO") {
				if ($#cmd != 0) { print_client($client, "ERR\n"); next; }
				print_client($client, "$do_pos_cur\n");
				next;
			}
			# --------------- Dome Stop ----------- #
			if ($cmd[0] eq "DOST") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 0) { print_client($client, "ERR\n"); next; }
				if ($do_state == DO_OFF) { print_client($client, "1\n"); next; }
				$do_state = DO_STOP;
				print_client($client, "1\n");
				next;
			}
			# ----- Dome Set Relative Position ---- #
			if ($cmd[0] eq "DOSR") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], -179.99, 180.00, 2)) {
					if ($do_state == DO_OFF) { print_client($client, "1\n"); next; }
					$do_pos = $cmd[1];
					print_client($client, "1\n");
					next;
				}
			}
			# ----- Dome Set Absolute Position ---- #
			if ($cmd[0] eq "DOSA") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (in_range($cmd[1], 0.00, 359.99, 2)) {
					if ($do_state == DO_OFF) { print_client($client, "1\n"); next; }
					$do_pos = $cmd[1];
					print_client($client, "1\n");
					next;
				}
			}
			# ----- Dome Go Relative Position ----- #
			if ($cmd[0] eq "DOGR") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 0) { print_client($client, "ERR\n"); next; }
				if (($do_state == DO_OFF) || ($do_state == DO_AUTO_PLUS) ||
				    ($do_state == DO_AUTO_MINUS) || ($do_state == DO_AUTO_STOP)) {
					print_client($client, "1\n");
					next;
				}
				if ($do_pos > 0) {
					$do_state = DO_SLEW_PLUS;
				} else {
					$do_state = DO_SLEW_MINUS;
				}
				$do_pos = ($do_pos_cur + $do_pos) % 360;
				$do_rel_moving_time = time();
				print_client($client, "1\n");
				next;
			}
			# ----- Dome Go Absolute Position ----- #
			if ($cmd[0] eq "DOGA") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 0) { print_client($client, "ERR\n"); next; }
				if (($do_state == DO_OFF) || ($do_state == DO_AUTO_PLUS) ||
				    ($do_state == DO_AUTO_MINUS) || ($do_state == DO_AUTO_STOP)) {
					print_client($client, "1\n");
					next;
				}
				if ($do_pos > ($do_pos_cur + 180.0) % 360) {
					$do_state = DO_SLEW_MINUS;
				} else {
					$do_state = DO_SLEW_PLUS;
				}
				$do_abs_moving_time = time();
				print_client($client, "1\n");
				next;
			}
			# ---------- Dome Automated ----------- #
			if ($cmd[0] eq "DOAM") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 0) { print_client($client, "ERR\n"); next; }
				if (($do_state == DO_OFF) ||
				    ($do_state == DO_SLEW_MINUS) ||
				    ($do_state == DO_SLEW_PLUS)) {
					print_client($client, "1\n");
					next;
				}
				if ($west) {
					$do_state = DO_AUTO_PLUS;
				} else {
					$do_state = DO_AUTO_MINUS;
				}
				$do_auto_moving_time = time();
				print_client($client, "1\n");
				next;
			}
			# ----- Flap Tube Open or Close ------- #
			if ($cmd[0] eq "FTOC") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($fl_tb_state == FL_TB_CLOSE) && ($cmd[1] eq "1")) {
					$fl_tb_state = FL_TB_OPENING;
					$fl_tb_opening_time = time();
				}
				if (($fl_tb_state == FL_TB_OPEN) && ($cmd[1] eq "0")) {
					$fl_tb_state = FL_TB_CLOSING;
					$fl_tb_closing_time = time();
				}
				print_client($client, "1\n");
				next;
			}
			# ----- Flap Coude Open or Close ------ #
			if ($cmd[0] eq "FCOC") {
				if (!$login) { print_client($client, "ERR\n"); next; }
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if (($fl_cd_state == FL_CD_CLOSE) && ($cmd[1] eq "1")) {
					$fl_cd_state = FL_CD_OPENING;
					$fl_cd_opening_time = time();
				}
				if (($fl_cd_state == FL_CD_OPEN) && ($cmd[1] eq "0")) {
					$fl_cd_state = FL_CD_CLOSING;
					$fl_cd_closing_time = time();
				}
				print_client($client, "1\n");
				next;
			}

			# ----- Global Meteo Data ----- #
			if ($cmd[0] eq "GLME") {
				# Hardcoded but these do not change rapidly
				my @values = ("8.87", "828.73", "63.19", "2.25", "9.43", "9.96", "10.90", "9.58");
				if ($#cmd == 0) {
					print_client($client, "@values\n");
					next;
				} elsif (($#cmd == 1) and in_range($cmd[1], 0, 7, 0)) {
					print_client($client, $values[$cmd[1]]."\n");
					next;
				}
				print_client($client, "ERR\n");
				next;
			}

			# ----- Global Login (passwod: 123) ----- #
			if ($cmd[0] eq "GLLG") {
				if ($#cmd != 1) { print_client($client, "ERR\n"); next; }
				if ($cmd[1] eq "123") {
					$login = 1;
					print_client($client, "1\n");
				} else {
					print_client($client, "0\n");
				}
				next;
			}
			# ----- Global System Status ----- #
			if ($cmd[0] eq "GLST") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				print_client($client, "$oil_state $te_state $ha_state $da_state $fo_state 0 $do_state $sl_state $fl_tb_state $fl_cd_state 0 0 0 0 $correction_model $state_bits @alarm_bits 0\n");
				next;
			}

			# ----- Telescope GMT ----- #
			if ($cmd[0] eq "GLUT") {
				if ($#cmd!=0) { print_client($client, "ERR\n"); next; }
				my ($s,$us) = gettimeofday();
				my $ascol_ut = sprintf "%s.%03d\n", strftime("%H%M%S", gmtime($s)), int($us/1000);
				print_client($client, $ascol_ut);
				next;
			}

			if ($line=~/quit|exit/i) {
				last;
			} else {
				print_client($client, "ERR\n");
			}
		}
		close $client;
		$login = 0;
		printf "[Connection closed]\n";
	}
}
main();
