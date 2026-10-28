"""Inspect retained native JPEG EXIF; never access a camera."""
import json
import math
import sys
from PIL import Image

results = []
for argument in sys.argv[1:]:
    requested, path = argument.split("=", 1)
    requested = float(requested)
    if not math.isfinite(requested) or requested <= 0:
        raise ValueError("Expected a positive finite requested duration")
    with Image.open(path) as image:
        image.load()
        actual = float(image.getexif().get_ifd(34665)[33434])
        if not math.isfinite(actual) or actual <= 0:
            raise ValueError("Invalid EXIF ExposureTime")
        error = actual - requested
        results.append(dict(requested_s=requested, exif_s=actual, error_ms=round(error * 1000, 3), abs_error_ms=round(abs(error) * 1000, 3), width=image.width, height=image.height, passed=abs(error) <= 0.050001))
if not results:
    raise ValueError("Provide requested=path arguments")
print(json.dumps(results, indent=2))
sys.exit(0 if all(result["passed"] for result in results) else 1)
