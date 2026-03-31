# INDIGO Guider Agent - Drift Detection Modes

Revision: 31.03.2026 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Overview

INDIGO Guider Agent provides four drift detection modes. The selected mode decides **how the drift is measured** from the guide frames.

* **Selection**
* **Weighted Selection**
* **DONUTS**
* **Centroid**

This is separate from the drift-correction mode described in [INDIGO_GUIDER_CORRECTION_MODES.md](INDIGO_GUIDER_CORRECTION_MODES.md):

* the **drift detection mode** measures the drift,
* the **drift correction mode** converts that measured drift into RA/Dec correction pulses.

In practice:

* Choose the **drift detection mode** by the **shape of the target and the image content**.
* Choose the **drift correction mode** by the **mount behavior**.
* If the guide graph is poor, decide first whether the problem is **bad drift measurement** or **bad correction behavior**.

As a rule:

* Start with **Selection** for ordinary star guiding, including mildly or slightly defocused stars.
* Use **Weighted Selection** when multi-star guiding is good but some stars are clearly noisier than others. It can also work well with slightly defocused stars.
* Use **DONUTS** for strongly defocused-star workflows or when hot pixels and cosmetic defects make star selection inconvenient.
* Use **Centroid** only for extended bright targets such as the Moon or planets.

## Mode 1: Selection

*Useful for: star guiding. **This is the default mode.***

### How It Works

Selection measures the centroid of one or more explicitly selected stars. For each selected star, the guider computes a local centroid inside the configured **Radius** and iteratively recenters the measurement on that star.

If multiple stars are selected, the guider forms a combined drift estimate from their measured motion. During guiding it rejects outliers before averaging the remaining drifts.

In simplified form for multi-star guiding, where $N$ is the number of accepted (non-outlier) stars:

```math
\Delta x \approx \frac{1}{N} \sum_{i=1}^{N} \Delta x_i, \qquad
\Delta y \approx \frac{1}{N} \sum_{i=1}^{N} \Delta y_i
```

This is the most general-purpose mode for ordinary guide stars.

### Parameters

* **Selection X** and **Selection Y** — Initial star coordinates.
* **Radius** — Measurement radius around each selected star.
* **Star count** — Number of guide stars used in multi-star mode.
* **Subframe** — Enables automatic subframing around the selected guide area for better performance and lower transfer load.
* **Include / Exclude regions** — Restrict where stars may be selected from and help avoid bad areas of the frame.

### When to Use

* **General star guiding** — Best starting point for most setups.
* **Slightly defocused stars** — Usually still works well as long as the star profile remains compact enough for stable centroiding.
* **Single-star guiding** — Simple and predictable.
* **Multi-star guiding with reasonably uniform star quality** — Works well when the chosen stars have similar SNR and stability.
* **Typical PI guiding workflows** — A very natural pairing with **Proportional-Integral** in [INDIGO_GUIDER_CORRECTION_MODES.md](INDIGO_GUIDER_CORRECTION_MODES.md).

### When Not to Use

* **Highly defocused stars** — **DONUTS** is usually better.
* **Extended targets** — Use **Centroid** for the Moon or planets.
* **Multi-star sets with very uneven star quality** — **Weighted Selection** is often better.
* **Strong field rotation across the frame** — Multi-star averaging can become less representative if the image geometry is changing.

---

## Mode 2: Weighted Selection

*Useful for: star guiding.*

### How It Works

Weighted Selection uses the same local centroid measurements as **Selection**, but combines multiple stars with **SNR-based weighting** instead of a simple average.

Internally, each accepted star contributes approximately with weight:

```math
w_i \propto \sqrt{\mathrm{SNR}_i}
```

and the combined drift is approximately:

```math
\Delta x \approx \frac{\sum w_i \Delta x_i}{\sum w_i}, \qquad
\Delta y \approx \frac{\sum w_i \Delta y_i}{\sum w_i}
```

Large outliers are still rejected before the final combination.

This means better stars influence the result more strongly, while weaker or noisier stars contribute less.

### Parameters

Weighted Selection uses the same parameters as **Selection**:

* **Selection X** and **Selection Y**
* **Radius**
* **Star count**
* **Subframe**
* **Include / Exclude regions**

### When to Use

* **Multi-star guiding with unequal stars** — Especially when some stars are much brighter or cleaner than others.
* **Slightly defocused stars** — Often still works well, especially when the selected stars remain distinct and reasonably compact.
* **Noisy guide frames** — The weighting reduces the influence of marginal stars.
* **Dense star fields** — Useful when a few strong stars are mixed with many weaker ones.
* **Pairing with PI or Hysteresis** — Often a good combination when the mount is fine but the measurement itself is noisy. See [INDIGO_GUIDER_CORRECTION_MODES.md](INDIGO_GUIDER_CORRECTION_MODES.md).

### When Not to Use

* **Single-star guiding** — It offers no practical advantage over **Selection**.
* **All selected stars are already similar** — Plain **Selection** is simpler and usually just as good.
* **Highly defocused or non-stellar targets** — Use **DONUTS** or **Centroid** instead.

---

## Mode 3: DONUTS

*Useful for: star fields, including defocused-star guiding.*

### How It Works

DONUTS computes a digest from the image structure rather than tracking one explicit star. It compares the current frame to the reference frame and estimates the shift of the whole star pattern.

In practical terms:

* it can use the **whole frame**, or
* it can use the configured **Include region** if **Use include region for DONUTS** is enabled.

This makes DONUTS tolerant of:

* defocused guide stars,
* hot pixels,
* hot columns,
* and other local cosmetic defects.

Because it is a global image-shift method, it needs the field pattern to remain consistent enough from frame to frame.

### Parameters

* **Edge clipping (px)** — Ignores image borders where artifacts, vignetting, or bright-edge stars may corrupt the digest.
* **Use include region for DONUTS** — Restricts DONUTS to the configured **Include region** instead of the whole frame.
* **Include region** — Optional region of interest when the feature above is enabled.

### When to Use

* **Strongly defocused-star guiding** — This is one of its main strengths.
* **Frames with cosmetic defects** — Hot pixels, hot columns, and similar issues are less problematic.
* **Wide star fields with many stars** — The global pattern can be very stable.
* **Cases where manual star selection is inconvenient** — DONUTS can avoid babysitting a single star.
* **Pairing with PI or Hysteresis** — Usually a sensible starting point from [INDIGO_GUIDER_CORRECTION_MODES.md](INDIGO_GUIDER_CORRECTION_MODES.md).

### When Not to Use

* **Bright stars near the frame edge** — Use **Edge clipping** or an include region, or switch to **Selection**.
* **Significant field rotation** — Poor polar alignment can make the global frame comparison less reliable.
* **Very low SNR** — During guiding, poor DONUTS SNR is explicitly rejected.
* **Extended bright targets** — Use **Centroid** for the Moon or planets.

---

## Mode 4: Centroid

*Useful for: bright extended targets.*

### How It Works

Centroid computes a **full-frame centroid** of the image brightness distribution. It does not try to identify a guide star. Instead, it tracks the center of light of the whole frame.

This is useful when the target itself occupies a substantial part of the frame, such as:

* the Moon,
* a planet,
* or another bright extended object.

It is **not** intended for ordinary star guiding.

### Parameters

Centroid does not require star-selection parameters. It uses the whole frame.

### When to Use

* **Lunar guiding**
* **Planetary guiding**
* **Other bright extended targets**
* **Simple PI or Hysteresis correction workflows** — Usually the most natural pairing from [INDIGO_GUIDER_CORRECTION_MODES.md](INDIGO_GUIDER_CORRECTION_MODES.md).

### When Not to Use

* **Ordinary guide stars** — Use **Selection**, **Weighted Selection**, or **DONUTS** instead.
* **Sparse star fields** — The full-frame centroid is not a star tracker.
* **Crowded star fields where only one star should matter** — Use **Selection**.

---

## Comparison Summary

| Mode | Best for | Main strengths | Main limitations |
|------|----------|----------------|------------------|
| **Selection** | General star guiding, including slight defocus | Simple, robust, good default, supports single- and multi-star workflows | Less ideal for very uneven star quality or heavy defocus |
| **Weighted Selection** | Multi-star guiding with unequal star quality, including slight defocus | Better stars influence the solution more strongly; good noise handling | No benefit for single-star guiding |
| **DONUTS** | Strongly defocused star fields, full-frame pattern tracking | Tolerates strong defocus and cosmetic defects; no explicit star lock needed | Can be affected by edge stars, low SNR, or field rotation |
| **Centroid** | Moon, planets, bright extended targets | Very simple whole-frame target tracking | Not suitable for ordinary star guiding |

---

## Cross-Reference: Detection Modes vs Guiding Modes

Detection modes and guiding modes solve different problems, so there is no single mandatory pairing. Still, some combinations are more natural than others:

| Detection mode | Usually pairs well with | Why |
|----------------|-------------------------|-----|
| **Selection** | **Proportional-Integral**, **Hysteresis**, **Linear Trend**, sometimes **Resist Switch** in Dec | General-purpose star measurement works with almost any correction strategy |
| **Weighted Selection** | **Proportional-Integral** or **Hysteresis** | Helpful when the measurement is noisier than the mount behavior itself |
| **DONUTS** | **Proportional-Integral** or **Hysteresis** | DONUTS often benefits from a straightforward, stable correction mode |
| **Centroid** | **Proportional-Integral** or **Hysteresis** | Extended-target guiding is usually better served by simple, direct correction |

Practical rule:

* If the drift estimate itself looks unstable, first reconsider the **detection mode**.
* If the drift estimate looks believable but the corrections overshoot or lag, reconsider the **guiding mode** in [INDIGO_GUIDER_CORRECTION_MODES.md](INDIGO_GUIDER_CORRECTION_MODES.md).

---

## Typical Starting Configurations

### Ordinary guide-star setup

* **Detection mode:** Selection
* **Guiding mode:** Proportional-Integral, P-only or with a small I term

### Multi-star setup with mixed star quality

* **Detection mode:** Weighted Selection
* **Guiding mode:** Proportional-Integral or Hysteresis

### Defocused guiding setup

* **Detection mode:** DONUTS for strong defocus, or Selection / Weighted Selection for slight defocus
* **Guiding mode:** Proportional-Integral or Hysteresis

### Lunar or planetary setup

* **Detection mode:** Centroid
* **Guiding mode:** Proportional-Integral or Hysteresis

---

Clear skies!
