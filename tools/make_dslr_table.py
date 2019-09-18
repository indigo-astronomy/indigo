#!/usr/bin/env python3

import argparse
import csv
import os
import sys


VENDOR_IDX = 0
PRODUCT_IDX = 1
NAME_IDX = 2
FLAGS_IDX = 3
WIDTH_IDX = 4
HEIGHT_IDX = 5
PIXEL_SIZE_IDX = 6
ALIAS_IDX = 7
COMMENT = 8


LINUX_GPHOTO2_HEADER = """
#ifndef DSLR_MODEL_INFO_H
#define DSLR_MODEL_INFO_H

/* Created by tools/make_dslr_table.py

   WARNING! All changes made in this file will be lost! */

static struct dslr_model_info {
  const char	*name;
  int		 width, height;
  float		 pixel_size;
} dslr_model_info[] = {
""".strip()
LINUX_GPHOTO2_FOOTER = """
  { NULL, 0, 0, 0 },
};

#endif				/* DSLR_MODEL_INFO_H */
""".strip("\n")


def make_linux_gphoto2(data, root):
    grouped = {}
    order = []
    for row in data:
        vendor = row[VENDOR_IDX].strip()
        if vendor.startswith("0x"):
            continue
        if vendor not in grouped:
            grouped[vendor] = []
            order.append(vendor)
        target = grouped[vendor]
        width = row[WIDTH_IDX].strip()
        height = row[HEIGHT_IDX].strip()
        pixel_size = row[PIXEL_SIZE_IDX].strip()
        target.append([
            row[NAME_IDX].strip().upper(),
            width,
            height,
            pixel_size,
        ])
        alias = row[ALIAS_IDX].strip()
        if alias:
            for name in alias.split("/"):
                target.append([
                    name.strip().upper(),
                    width,
                    height,
                    pixel_size,
                ])

    def sort(rows): return sorted(rows, key=lambda x: x[0], reverse=True)
    sorted_data = []
    for key in order:
        sorted_data.extend(sort(grouped[key]))

    with open(os.path.join(root, "dslr_model_info.h"), "w") as fp:
        print(LINUX_GPHOTO2_HEADER, file=fp)
        for record in sorted_data:
            print('  {{ "{}", {}, {}, {} }},'.format(*record), file=fp)
        print(LINUX_GPHOTO2_FOOTER, file=fp)


PTP_HEADER = """
#ifndef PTP_CAMERA_MODEL_H
#define PTP_CAMERA_MODEL_H

/* Created by tools/make_dslr_table.py

   WARNING! All changes made in this file will be lost! */

static ptp_camera_model CAMERA[] = {
""".strip()

PTP_FOOTER = """
  { 0 },
};

#endif				/* PTP_CAMERA_MODEL_H */
""".strip("\n")


def make_ptp(data, root):
    filtered_data = []
    for row in data:
        vendor = row[VENDOR_IDX].strip()
        if not vendor.startswith("0x"):
            vendor = "{}_VID".format(vendor.upper())
        product = row[PRODUCT_IDX].strip()
        if not product:
            # product id is unknown
            continue
        flags = row[FLAGS_IDX].strip()
        if not flags:
            # flags is unknown
            continue
        filtered_data.append([
            vendor,
            product,
            row[NAME_IDX].strip(),
            flags,
            row[WIDTH_IDX].strip(),
            row[HEIGHT_IDX].strip(),
            row[PIXEL_SIZE_IDX].strip(),
        ])
    sorted_data = sorted(filtered_data, key=lambda x: (x[0], x[1]))

    with open(os.path.join(root, "ptp_camera_model.h"), "w") as fp:
        print(PTP_HEADER, file=fp)
        for record in sorted_data:
            print('  {{ {}, {}, "{}", {}, {}, {}, {} }},'.format(*record), file=fp)
        print(PTP_FOOTER, file=fp)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source",
                        default="./data/dslr.csv",
                        type=argparse.FileType("r"))
    parser.add_argument("--ignore-linux-gphoto2", action="store_true")
    parser.add_argument("--linux-gphoto2",
                        default="indigo_linux_drivers/ccd_gphoto2")
    parser.add_argument("--ignore-ptp", action="store_true")
    parser.add_argument("--ptp",
                        default="indigo_drivers/ccd_ptp")

    args = parser.parse_args()

    data = list(csv.reader(args.source, skipinitialspace=True))[1:]

    if not args.ignore_linux_gphoto2:
        make_linux_gphoto2(data, args.linux_gphoto2)

    if not args.ignore_ptp:
        make_ptp(data, args.ptp)

    return 0


if __name__ == "__main__":
    sys.exit(main())
