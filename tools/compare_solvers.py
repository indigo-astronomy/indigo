#!/usr/bin/env python3

# Solve one image with the INDIGO quad solver and with astrometry.net and compare the results.
#
# The INDIGO solver runs as build/bin/indigo_solver with the indexes of the Solver agent
# (~/.indigo/solver). astrometry.net runs the way the Astrometry agent runs it: image2xy
# extracts the stars from a FITS image and solve-field solves the star list with the indexes
# listed in ~/.indigo/astrometry/astrometry.cfg. Both directories follow INDIGO_CACHE_BASE.
#
# solve-field gets FITS through image2xy only, so other images are converted first: INDIGO RAW
# with numpy, JPEG, PNG and TIFF with numpy and Pillow. Camera raw files are solved by the INDIGO
# solver only.
#
# Both solutions are reported in J2000: the centre, the angle (up is east of north, as in
# AGENT_PLATESOLVER_WCS), the scale per unbinned pixel, the field size and the parity, then their
# differences. The exit code is 0 when both solved and agree within the tolerances, 1 when they
# disagree, 2 when at least one of them did not solve and 3 on wrong arguments or missing tools.

import argparse
import json
import math
import os
import re
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time

REPOSITORY = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

RAW_MONO8 = 0x31574152
RAW_MONO16 = 0x32574152
RAW_RGB24 = 0x33574152
RAW_RGBA32 = 0x36424752
RAW_ABGR32 = 0x36524742
RAW_RGB48 = 0x36574152


class ToolError(Exception):
	pass


def cache_dir(name):
	base = os.environ.get("INDIGO_CACHE_BASE")
	if base:
		return os.path.join(base, name)
	return os.path.join(os.path.expanduser("~"), ".indigo", name)


def image_kind(path):
	with open(path, "rb") as file:
		head = file.read(12)
	if head.startswith(b"SIMPLE"):
		return "fits"
	if len(head) >= 4 and struct.unpack("<I", head[:4])[0] in (RAW_MONO8, RAW_MONO16, RAW_RGB24, RAW_RGBA32, RAW_ABGR32, RAW_RGB48):
		return "raw"
	if head.startswith(b"\xff\xd8"):
		return "jpeg"
	if head.startswith(b"\x89PNG"):
		return "png"
	if head.startswith(b"II*\x00") or head.startswith(b"MM\x00*"):
		return "tiff"
	return "camera raw"


def mono_pixels(path, kind):
	import numpy
	if kind == "raw":
		with open(path, "rb") as file:
			signature, width, height = struct.unpack("<III", file.read(12))
			layout = {
				RAW_MONO8: (numpy.uint8, 1),
				RAW_MONO16: (numpy.dtype("<u2"), 1),
				RAW_RGB24: (numpy.uint8, 3),
				RAW_RGBA32: (numpy.uint8, 4),
				RAW_ABGR32: (numpy.uint8, 4),
				RAW_RGB48: (numpy.dtype("<u2"), 3),
			}[signature]
			pixels = numpy.fromfile(file, dtype=layout[0], count=width * height * layout[1])
		if pixels.size != width * height * layout[1]:
			raise ToolError("%s: truncated INDIGO RAW image" % path)
		pixels = pixels.reshape(height, width, layout[1]).astype(numpy.float32)
		if signature == RAW_RGBA32:
			pixels = pixels[:, :, :3]
		elif signature == RAW_ABGR32:
			pixels = pixels[:, :, 1:]
		return pixels.mean(axis=2)
	try:
		from PIL import Image
	except ImportError:
		raise ToolError("Pillow is needed to convert %s images for astrometry.net" % kind)
	with Image.open(path) as image:
		pixels = numpy.asarray(image).astype(numpy.float32)
	if pixels.ndim == 3:
		pixels = pixels[:, :, :3].mean(axis=2)
	return pixels


# Writes the image as 16-bit FITS for image2xy; the rows keep their order, as in the FITS file the Astrometry agent
# writes and as the INDIGO solver reads FITS, so both report the same angle and parity
def write_fits(pixels, path):
	import numpy
	low, high = float(pixels.min()), float(pixels.max())
	scaled = (pixels - low) * (65535.0 / (high - low)) if high > low else pixels * 0
	data = (scaled.round().astype(numpy.int32) - 32768).astype(">i2")
	cards = ["SIMPLE  =                    T", "BITPIX  =                   16", "NAXIS   =                    2", "NAXIS1  = %20d" % pixels.shape[1], "NAXIS2  = %20d" % pixels.shape[0], "BZERO   =                32768", "BSCALE  =                    1", "END"]
	header = "".join(card.ljust(80) for card in cards)
	header = header.ljust((len(header) + 2879) // 2880 * 2880)
	body = data.tobytes()
	with open(path, "wb") as file:
		file.write(header.encode("ascii"))
		file.write(body)
		file.write(b"\0" * ((2880 - len(body) % 2880) % 2880))


def run(command, timeout, verbose):
	if verbose:
		print("> " + " ".join(command), file=sys.stderr)
	started = time.monotonic()
	# solve-field runs astrometry-engine as a child, so the whole process group is killed on a timeout or an interrupt
	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace", start_new_session=True)
	try:
		output, _ = process.communicate(timeout=timeout)
		code = process.returncode
	except subprocess.TimeoutExpired:
		os.killpg(process.pid, signal.SIGKILL)
		output, _ = process.communicate()
		output += "\ntimed out after %g s\n" % timeout
		code = -1
	except BaseException:
		os.killpg(process.pid, signal.SIGKILL)
		process.wait()
		raise
	if verbose:
		sys.stderr.write(output)
	return code, output, time.monotonic() - started


def solve_indigo(args, image):
	command = [args.indigo_solver, "-i", args.solver_index_dir, "-D", str(args.downsample), "-c", str(args.cpu_limit)]
	if args.scale:
		command += ["-s", repr(args.scale / 3600)]
	if args.ra is not None:
		command += ["-r", repr(args.ra), "-d", repr(args.dec), "-R", repr(args.radius)]
	if args.indigo_index:
		command += ["-x", args.indigo_index]
	command.append(image)
	code, output, elapsed = run(command, args.cpu_limit + 60, args.verbose)
	result = {"solver": "INDIGO", "solved": False, "time": elapsed}
	values = {}
	for line in output.splitlines():
		match = re.match(r"(RA|Dec|Angle|Scale|Field|Parity|Index):\s+(.*)", line)
		if match:
			values[match.group(1)] = match.group(2)
	if code != 0 or "RA" not in values:
		lines = [line for line in output.splitlines() if line.strip()]
		result["message"] = lines[-1] if lines else "exit code %d" % code
		return result
	field = re.match(r"([-\d.]+)° x ([-\d.]+)°", values["Field"])
	result.update({
		"solved": True,
		"ra": float(values["RA"].split()[0]) * 15,
		"dec": float(values["Dec"].split("°")[0]),
		"angle": float(values["Angle"].rstrip("°")),
		"scale": float(values["Scale"].split("°")[0]) * 3600,
		"width": float(field.group(1)),
		"height": float(field.group(2)),
		"parity": int(values["Parity"]),
		"index": values["Index"],
	})
	return result


def solve_astrometry(args, image, kind, work):
	result = {"solver": "astrometry.net", "solved": False, "time": 0}
	config = os.path.join(args.astrometry_index_dir, "astrometry.cfg")
	if not os.path.isfile(config):
		result["message"] = "%s not found, install indexes with the Astrometry agent" % config
		return result
	if kind == "camera raw":
		result["message"] = "camera raw files are not supported, convert the image to FITS"
		return result
	fits = image
	if kind != "fits":
		started = time.monotonic()
		try:
			fits = os.path.join(work, "image.fits")
			write_fits(mono_pixels(image, kind), fits)
		except ImportError:
			result["message"] = "numpy is needed to convert %s images for astrometry.net" % kind
			return result
		except (ToolError, KeyError, ValueError) as error:
			result["message"] = str(error)
			return result
		result["time"] += time.monotonic() - started
	xy = os.path.join(work, "image.xy")
	command = ["image2xy", "-O", "-o", xy]
	if args.downsample > 1:
		command += ["-d", str(args.downsample)]
	command.append(fits)
	code, output, elapsed = run(command, 120, args.verbose)
	result["time"] += elapsed
	if code != 0:
		result["message"] = "image2xy failed: " + (output.strip().splitlines() or ["exit code %d" % code])[-1]
		return result
	command = ["solve-field", "--overwrite", "--crpix-center", "--no-plots", "--no-remove-lines", "--no-verify-uniformize", "--sort-column", "FLUX", "--uniformize", "0", "--config", config, "--dir", work, "--cpulimit", str(args.cpu_limit)]
	if args.scale:
		command += ["--scale-units", "arcsecperpix", "--scale-low", "%.3f" % (args.scale * 0.85), "--scale-high", "%.3f" % (args.scale * 1.15)]
	if args.ra is not None:
		command += ["--ra", repr(args.ra * 15), "--dec", repr(args.dec), "--radius", repr(args.radius)]
	command.append(xy)
	code, output, elapsed = run(command, args.cpu_limit + 60, args.verbose)
	result["time"] += elapsed
	wcs = os.path.join(work, "image.wcs")
	if not os.path.isfile(wcs):
		if "Total CPU time limit reached" in output:
			result["message"] = "CPU time limit reached"
		elif code != 0:
			result["message"] = "solve-field failed: " + (output.strip().splitlines() or ["exit code %d" % code])[-1]
		else:
			result["message"] = "No solution found"
		return result
	code, info, _ = run(["wcsinfo", wcs], 30, False)
	values = dict(line.split(None, 1) for line in info.splitlines() if len(line.split(None, 1)) == 2)
	parity = re.search(r"Field parity: (pos|neg)", output)
	index = re.search(r"solved with index index-(\d+)", output)
	try:
		scale = float(values["pixscale"])
		result.update({
			"solved": True,
			"ra": float(values["ra_center"]),
			"dec": float(values["dec_center"]),
			"angle": float(values["orientation"]),
			"scale": scale,
			"width": float(values["imagew"]) * scale / 3600,
			"height": float(values["imageh"]) * scale / 3600,
			"parity": (1 if parity.group(1) == "pos" else -1) if parity else 0,
			"index": index.group(1) if index else "?",
		})
	except (KeyError, ValueError):
		result["message"] = "wcsinfo failed to read %s" % wcs
	return result


def separation(ra1, dec1, ra2, dec2):
	ra1, dec1, ra2, dec2 = (math.radians(value) for value in (ra1, dec1, ra2, dec2))
	value = math.sin(dec1) * math.sin(dec2) + math.cos(dec1) * math.cos(dec2) * math.cos(ra1 - ra2)
	return math.degrees(math.acos(max(-1.0, min(1.0, value))))


def hms(degrees):
	hours = (degrees / 15) % 24
	h, rest = divmod(round(hours * 360000), 360000)
	m, s = divmod(rest, 6000)
	return "%02d:%02d:%05.2f" % (h, m, s / 100)


def dms(degrees):
	d, rest = divmod(round(abs(degrees) * 36000), 36000)
	m, s = divmod(rest, 600)
	return "%s%02d:%02d:%04.1f" % ("-" if degrees < 0 else "+", d, m, s / 10)


def report(image, results, comparison):
	print("Image: %s" % image)
	print()
	print("%-12s %-28s %-28s" % ("", results[0]["solver"], results[1]["solver"]))
	rows = [
		("Solved", lambda r: "yes" if r["solved"] else "no: " + r.get("message", "")),
		("Time", lambda r: "%.2f s" % r["time"]),
		("RA", lambda r: "%s (%.5f°)" % (hms(r["ra"]), r["ra"])),
		("Dec", lambda r: "%s (%+.5f°)" % (dms(r["dec"]), r["dec"])),
		("Angle", lambda r: "%.3f°" % r["angle"]),
		("Scale", lambda r: "%.4f\"/px" % r["scale"]),
		("Field", lambda r: "%.4f° x %.4f°" % (r["width"], r["height"])),
		("Parity", lambda r: "%+d" % r["parity"]),
		("Index", lambda r: r["index"]),
	]
	for label, value in rows:
		cells = []
		for result in results:
			try:
				cells.append(value(result))
			except KeyError:
				cells.append("-")
		print("%-12s %-28s %-28s" % (label + ":", cells[0], cells[1]))
	if comparison:
		print()
		print("Centre difference: %.2f\" (RA %+.2f\", Dec %+.2f\")" % (comparison["centre"], comparison["ra"], comparison["dec"]))
		print("Angle difference:  %+.3f°" % comparison["angle"])
		print("Scale difference:  %+.4f%%" % comparison["scale"])
		print("Parity:            %s" % ("same" if comparison["parity"] else "different"))
		print()
		print("Result: %s" % ("solutions agree" if comparison["agree"] else "solutions DIFFER: " + ", ".join(comparison["problems"])))


def main():
	parser = argparse.ArgumentParser(description="Solve an image with the INDIGO solver and with astrometry.net and compare the results.")
	parser.add_argument("image", help="INDIGO RAW, FITS, JPEG, PNG or TIFF image; camera raw files are solved by the INDIGO solver only")
	parser.add_argument("-s", "--scale", type=float, default=0, help="pixel scale hint in arcsec per pixel, 0 = unknown (default: 0)")
	parser.add_argument("-r", "--ra", type=float, help="RA hint in hours, J2000")
	parser.add_argument("-d", "--dec", type=float, help="Dec hint in degrees, J2000")
	parser.add_argument("-R", "--radius", type=float, default=10, help="search radius around the RA/Dec hint in degrees (default: 10)")
	parser.add_argument("-D", "--downsample", type=int, default=2, help="bin the image n x n before the star detection, the same for both (default: 2)")
	parser.add_argument("-c", "--cpu-limit", type=int, default=180, help="CPU limit of each solver in seconds (default: 180)")
	parser.add_argument("-x", "--indigo-index", help="comma separated scales of the INDIGO indexes to use, in degrees (default: all installed)")
	parser.add_argument("--indigo-solver", default=os.path.join(REPOSITORY, "build", "bin", "indigo_solver"), help="indigo_solver executable (default: build/bin/indigo_solver of this checkout)")
	parser.add_argument("--solver-index-dir", default=cache_dir("solver"), help="INDIGO index directory (default: %(default)s)")
	parser.add_argument("--astrometry-index-dir", default=cache_dir("astrometry"), help="astrometry.net index directory with astrometry.cfg (default: %(default)s)")
	parser.add_argument("--center-tolerance", type=float, default=10, help="accepted centre difference in arcsec (default: 10)")
	parser.add_argument("--angle-tolerance", type=float, default=0.1, help="accepted angle difference in degrees (default: 0.1)")
	parser.add_argument("--scale-tolerance", type=float, default=0.1, help="accepted scale difference in percent (default: 0.1)")
	parser.add_argument("--json", action="store_true", help="print the results as JSON")
	parser.add_argument("--keep", action="store_true", help="keep the astrometry.net work directory and print its path")
	parser.add_argument("-v", "--verbose", action="store_true", help="show the commands and their output")
	args = parser.parse_args()
	# a terminated run still kills its solvers and removes its work directory
	signal.signal(signal.SIGTERM, lambda number, frame: sys.exit(128 + number))
	if (args.ra is None) != (args.dec is None):
		parser.error("the RA and the Dec hint go together")
	if args.downsample < 1:
		parser.error("the downsample factor is at least 1")
	if not os.path.isfile(args.image):
		print("%s: no such file" % args.image, file=sys.stderr)
		return 3
	missing = [tool for tool in (args.indigo_solver, "image2xy", "solve-field", "wcsinfo") if shutil.which(tool) is None]
	if missing:
		print("missing executables: %s%s" % (", ".join(missing), " (run 'make' to build indigo_solver)" if args.indigo_solver in missing else ""), file=sys.stderr)
		return 3
	image = os.path.abspath(args.image)
	kind = image_kind(image)
	work = tempfile.mkdtemp(prefix="compare_solvers_")
	try:
		results = [solve_indigo(args, image), solve_astrometry(args, image, kind, work)]
	finally:
		if args.keep:
			print("astrometry.net files kept in %s" % work, file=sys.stderr)
		else:
			shutil.rmtree(work, ignore_errors=True)
	comparison = None
	if results[0]["solved"] and results[1]["solved"]:
		a, b = results
		ra = (a["ra"] - b["ra"] + 180) % 360 - 180
		comparison = {
			"centre": separation(a["ra"], a["dec"], b["ra"], b["dec"]) * 3600,
			"ra": ra * math.cos(math.radians(b["dec"])) * 3600,
			"dec": (a["dec"] - b["dec"]) * 3600,
			"angle": (a["angle"] - b["angle"] + 180) % 360 - 180,
			"scale": (a["scale"] / b["scale"] - 1) * 100,
			"parity": a["parity"] == b["parity"],
		}
		problems = []
		if comparison["centre"] > args.center_tolerance:
			problems.append("centre")
		if abs(comparison["angle"]) > args.angle_tolerance:
			problems.append("angle")
		if abs(comparison["scale"]) > args.scale_tolerance:
			problems.append("scale")
		if not comparison["parity"]:
			problems.append("parity")
		comparison["problems"] = problems
		comparison["agree"] = not problems
	if args.json:
		print(json.dumps({"image": image, "kind": kind, "results": results, "comparison": comparison}, indent=2, ensure_ascii=False))
	else:
		report(image, results, comparison)
	if comparison is None:
		return 2
	return 0 if comparison["agree"] else 1


if __name__ == "__main__":
	sys.exit(main())
