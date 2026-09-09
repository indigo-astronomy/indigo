// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
#ifndef GUIDER_ASI_FAKE_SDK_H
#define GUIDER_ASI_FAKE_SDK_H
#include <stdbool.h>
#include <stdatomic.h>
#include <USB2ST4_Conv.h>
#include <indigo/indigo_usb_utils.h>
// Primary contract: vendor USB2ST4_Conv.h in the driver SDK directory.
// It documents stable IDs, lifecycle errors and independent direction ON/OFF.
// No implicit opposite-direction stop or automatic pulse duration is invented.
extern atomic_int asi_present, asi_attached, asi_opened, asi_closed, asi_invalid_io;
extern atomic_int asi_fail_open, asi_fail_on, asi_fail_off, asi_fail_products, asi_fail_register;
extern atomic_int asi_relays;
extern double asi_on_time[4], asi_off_time[4];
void asi_fake_arrival(void);
void asi_fake_removal(void);
#endif
