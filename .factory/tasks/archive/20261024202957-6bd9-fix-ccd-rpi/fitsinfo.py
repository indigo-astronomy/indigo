#!/usr/bin/env python3
# Prints size and per-channel statistics of an 8 bit INDIGO FITS image (NAXIS3 = 3 for RGB)
import sys


def read(path):
	data = open(path, 'rb').read()
	header = {}
	offset = 0
	while True:
		card = data[offset:offset + 80].decode('ascii', 'replace')
		offset += 80
		if card.startswith('END'):
			break
		if '=' in card[:10]:
			key = card[:8].strip()
			value = card[10:].split('/')[0].strip().strip("'").strip()
			header[key] = value
	offset = (offset + 2879) // 2880 * 2880
	return header, data[offset:]


def main():
	header, pixels = read(sys.argv[1])
	width = int(header['NAXIS1'])
	height = int(header['NAXIS2'])
	planes = int(header.get('NAXIS3', '1'))
	plane_size = width * height
	means = []
	for p in range(planes):
		plane = pixels[p * plane_size:(p + 1) * plane_size]
		sample = plane[::97]
		means.append(sum(sample) / len(sample))
	diff = 0
	if planes == 3:
		r = pixels[0:plane_size:97]
		g = pixels[plane_size:2 * plane_size:97]
		diff = sum(abs(a - b) for a, b in zip(r, g)) / len(r)
	print('size %dx%dx%d bitpix %s exptime %s means %s rg_diff %.2f' % (width, height, planes, header['BITPIX'], header.get('EXPTIME'), ' '.join('%.1f' % m for m in means), diff))


main()
