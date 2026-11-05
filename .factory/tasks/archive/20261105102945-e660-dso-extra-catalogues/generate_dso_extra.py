#!/usr/bin/env python3
"""Generates the Sharpless, Barnard, van den Bergh, Abell PN, Collinder and Melotte entries of indigocat_dso.c.

Usage: generate_dso_extra.py WORK_DIR CATALOGUE_C OUTPUT_ROWS REPORT
WORK_DIR holds v20/catalog.dat (VII/20), v220A/barnard.dat (VII/220A), v21/catalog.dat (VII/21) and receives simbad.json.
"""
import gzip, json, math, os, re, sys, urllib.parse, urllib.request

WORK, CATALOGUE, OUTPUT, REPORT = sys.argv[1:5]
TAP = 'https://simbad.harvard.edu/simbad/sim-tap/sync'
MERGE_LIMIT = 70  # arcmin
EXISTING = {'Sh2-276': 'Sh 2-276', 'Mel22': 'Cl Melotte 22', 'Mel25': 'C41'}
NEBULAR = {'NEBULA', 'REFLECTION_NEBULA', 'EMISSION_NEBULA', 'HII_REGION', 'STAR_CLUSTER_NEBULA', 'SUPERNOVA_REMNANT', 'DARK_NEBULA', 'PLANETARY_NEBULA'}
CLUSTERS = {'OPEN_CLUSTER', 'GLOBULAR_CLUSTER', 'STAR_CLUSTER_NEBULA', 'ASSOCIATION_OF_STARS'}
PATTERNS = {'Sh2': ('SH  2-%', r'SH  2-(\d+)'), 'vdB': ('VDB %', r'VDB\s+(\d+)'), 'Abell': ('PN A66 %', r'PN A66\s+(\d+)'), 'Cr': ('Cl Collinder %', r'Cl Collinder\s+(\d+)'), 'Mel': ('Cl Melotte %', r'Cl Melotte\s+(\d+)')}


def tap(query):
    data = urllib.parse.urlencode({'request': 'doQuery', 'lang': 'adql', 'format': 'json', 'query': query}).encode()
    return json.load(urllib.request.urlopen(urllib.request.Request(TAP, data=data), timeout=120))['data']


def simbad():
    path = os.path.join(WORK, 'simbad.json')
    if os.path.exists(path):
        return json.load(open(path))
    out = {}
    for key, (like, _) in PATTERNS.items():
        rows = tap(f"SELECT i.id, b.ra, b.dec, b.otype, b.galdim_majaxis, b.oid FROM ident i JOIN basic b ON b.oid = i.oidref WHERE i.id LIKE '{like}' AND b.ra IS NOT NULL")
        cross = tap(f"SELECT i.id, j.id FROM ident i JOIN ident j ON i.oidref = j.oidref WHERE i.id LIKE '{like}' AND (j.id LIKE 'NGC %' OR j.id LIKE 'IC %')")
        out[key] = {'rows': rows, 'cross': cross}
    json.dump(out, open(path, 'w'))
    return out


def galactic_to_j2000(l, b):
    l, b = math.radians(l), math.radians(b)
    ngp_ra, ngp_dec, l_ncp = math.radians(192.85948), math.radians(27.12825), math.radians(122.93192)
    dec = math.asin(math.sin(ngp_dec) * math.sin(b) + math.cos(ngp_dec) * math.cos(b) * math.cos(l_ncp - l))
    ra = ngp_ra + math.atan2(math.cos(b) * math.sin(l_ncp - l), math.cos(ngp_dec) * math.sin(b) - math.sin(ngp_dec) * math.cos(b) * math.cos(l_ncp - l))
    return math.degrees(ra) % 360, math.degrees(dec)


def b1950_to_j2000(ra, dec):
    rotation = ((0.9999256782, -0.0111820611, -0.0048579477), (0.0111820610, 0.9999374784, -0.0000271765), (0.0048579479, -0.0000271474, 0.9999881997))
    a, d = math.radians(ra), math.radians(dec)
    v = (math.cos(d) * math.cos(a), math.cos(d) * math.sin(a), math.sin(d))
    x = [sum(rotation[i][j] * v[j] for j in range(3)) for i in range(3)]
    return math.degrees(math.atan2(x[1], x[0])) % 360, math.degrees(math.asin(x[2]))


def separation(ra1, dec1, ra2, dec2):
    a = math.sin(math.radians(dec1 - dec2) / 2) ** 2 + math.cos(math.radians(dec1)) * math.cos(math.radians(dec2)) * math.sin(math.radians(ra1 - ra2) / 2) ** 2
    return math.degrees(2 * math.asin(math.sqrt(a))) * 60


source = open(CATALOGUE, errors='replace').read()
rows = re.findall(r'\{ "([^"]+)", (\w+), ([-\d.]+), ([-\d.]+), ([-\d.]+), ([-\d.]+), ([-\d.]+), ([-\d.]+), "([^"]*)" \}', source)
existing = [{'id': r[0], 'type': r[1], 'ra': float(r[2]) * 15, 'dec': float(r[3]), 'r': max(float(r[5]), float(r[6]))} for r in rows]
by_id = {e['id']: e for e in existing}
data = simbad()

new = []  # catalogue id, type, ra degrees, dec, diameter arcmin, source
skipped = []
cross = {}
for key, part in data.items():
    for a, b in part['cross']:
        m = re.fullmatch(PATTERNS[key][1], a)
        if m:
            cross.setdefault(f'{key}{"-" if key == "Sh2" else ""}{int(m[1])}', []).append(re.sub(r'\s+', '', b))


def simbad_rows(key):
    out = {}
    for row in data[key]['rows']:
        m = re.fullmatch(PATTERNS[key][1], row[0])
        if m:
            out[int(m[1])] = row
    return out


# Sharpless: VII/20 numbers, diameters and B1950 positions (SIMBAD positions of several Sh2 objects are up to 15 degrees off)
for line in gzip.open(os.path.join(WORK, 'v20/catalog.dat.gz'), 'rt'):
    number = int(line[0:4])
    diam = float(line[48:52])
    ra = (int(line[34:36]) + int(line[36:38]) / 60 + int(line[38:41]) / 36000) * 15
    dec = (int(line[42:44]) + int(line[44:46]) / 60 + int(line[46:48]) / 3600) * (-1 if line[41] == '-' else 1)
    ra, dec = b1950_to_j2000(ra, dec)
    new.append((f'Sh2-{number}', 'HII_REGION', ra, dec, diam, 'VII/20'))

# Barnard: VII/220A J2000 positions and diameters; a missing diameter is left as 0
for line in open(os.path.join(WORK, 'v220A/barnard.dat')):
    number = line[1:5].strip()
    sec = line[28:30].strip()
    ra = (int(line[22:24]) + int(line[25:27]) / 60 + (int(sec) if sec else 0) / 3600) * 15
    dec = (int(line[33:35]) + int(line[36:38]) / 60) * (-1 if line[32] == '-' else 1)
    diam = line[39:44].strip()
    new.append((f'B{number}', 'DARK_NEBULA', ra, dec, float(diam) if diam else 0, 'VII/220A'))

# van den Bergh: VII/21 numbers and radii, SIMBAD position of the illuminating star, otherwise the catalogue galactic coordinates
vdb = simbad_rows('vdB')
for line in open(os.path.join(WORK, 'v21/catalog.dat')):
    number = int(line[1:4])
    radii = [float(v) for v in (line[70:75].strip(), line[76:81].strip()) if v]
    diam = 2 * max(radii) if radii else 0
    if number in vdb:
        ra, dec = vdb[number][1], vdb[number][2]
    else:
        ra, dec = galactic_to_j2000(float(line[24:29]), float(line[29:34]))
    new.append((f'vdB{number}', 'REFLECTION_NEBULA', ra, dec, diam, 'VII/21'))

# Abell planetary nebulae, Collinder and Melotte clusters: SIMBAD
for number, row in sorted(simbad_rows('Abell').items()):
    new.append((f'Abell{number}', 'PLANETARY_NEBULA', row[1], row[2], row[4] or 0, 'SIMBAD'))
for key in ('Cr', 'Mel'):
    for number, row in sorted(simbad_rows(key).items()):
        if row[3] == 'err':
            skipped.append((f'{key}{number}', 'SIMBAD marks the identification as an error'))
            continue
        new.append((f'{key}{number}', 'GLOBULAR_CLUSTER' if row[3] == 'GlC' else 'OPEN_CLUSTER', row[1], row[2], row[4] or 0, 'SIMBAD'))

accepted, merged = [], []
for item in new:
    name, kind, ra, dec, diam, origin = item
    if name in EXISTING:
        merged.append((name, EXISTING[name], 'already in the catalogue'))
        continue
    # SIMBAD may identify an object wrongly, the catalogue position has to agree
    same = [c for c in cross.get(name, []) if c in by_id and separation(ra, dec, by_id[c]['ra'], by_id[c]['dec']) <= max(MERGE_LIMIT, diam, by_id[c]['r'])]
    if same:
        merged.append((name, same[0], 'same SIMBAD object'))
        continue
    # the same object under another name: close together and of a similar size, in the same broad class
    group = NEBULAR if kind in NEBULAR else CLUSTERS
    best = None
    for e in existing:
        if e['type'] not in group:
            continue
        sep = separation(ra, dec, e['ra'], e['dec'])
        if diam > 0 and e['r'] > 0:
            close = sep <= min(0.5 * max(diam, e['r']), MERGE_LIMIT) and max(diam, e['r']) <= 3 * min(diam, e['r'])
        else:
            close = sep <= 1.0
        if close and (best is None or sep < best[0]):
            best = (sep, e['id'], e['r'])
    if best:
        merged.append((name, best[1], f'{best[0]:.1f}\' apart, {diam:g}\' and {best[2]:g}\' across'))
        continue
    accepted.append(item)

with open(OUTPUT, 'w') as f:
    for name, kind, ra, dec, diam, _ in accepted:
        f.write(f'\t{{ "{name}", {kind}, {ra / 15:.4f}, {dec:.4f}, 0.0, {diam:.2f}, {diam:.2f}, 0, "" }},\n')
with open(REPORT, 'w') as f:
    counts = {}
    for item in new:
        k = re.match(r'[A-Za-z]+', item[0])[0]
        counts.setdefault(k, [0, 0, 0])[0] += 1
    for item in accepted:
        k = re.match(r'[A-Za-z]+', item[0])[0]
        counts[k][1] += 1
        if item[4] == 0:
            counts[k][2] += 1
    f.write('catalogue read added without_size\n')
    for k, v in counts.items():
        f.write(f'{k} {v[0]} {v[1]} {v[2]}\n')
    f.write(f'total {len(new)} {len(accepted)} {sum(v[2] for v in counts.values())}\n\nmerged\n')
    for m in merged:
        f.write(' | '.join(m) + '\n')
    f.write('\nskipped\n')
    for s in skipped:
        f.write(' | '.join(s) + '\n')
