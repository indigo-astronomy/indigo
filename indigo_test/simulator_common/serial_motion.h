// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#ifndef SERIAL_MOTION_H
#define SERIAL_MOTION_H
#include <time.h>
#include <math.h>

typedef struct {
	double position, target, origin, started, duration;
} serial_motion;

static double serial_motion_time(void) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec + now.tv_nsec / 1e9;
}

static double serial_motion_update(serial_motion *motion) {
	if (motion->duration > 0) {
		double fraction = (serial_motion_time() - motion->started) / motion->duration;
		if (fraction >= 1) {
			motion->position = motion->target;
			motion->duration = 0;
		} else {
			motion->position = motion->origin + (motion->target - motion->origin) * fraction;
		}
	}
	return motion->position;
}

static void serial_motion_sync(serial_motion *motion, double position) {
	motion->position = motion->target = motion->origin = position;
	motion->duration = 0;
}

static void serial_motion_start(serial_motion *motion, double target, double speed) {
	serial_motion_update(motion);
	motion->origin = motion->position;
	motion->target = target;
	motion->started = serial_motion_time();
	motion->duration = fabs(target - motion->origin) / speed;
	if (motion->duration > 0 && motion->duration < 0.5) {
		motion->duration = 0.5;
	}
}

static void serial_motion_stop(serial_motion *motion) {
	serial_motion_sync(motion, serial_motion_update(motion));
}
#endif
