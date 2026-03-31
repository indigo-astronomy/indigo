# INDIGO Guider Agent - Dithering

Revision: 31.03.2026 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Overview

Dithering is the deliberate shifting of the guide star's reference position between frames. The guider moves the telescope by a small, controlled amount after each (or every N-th) frame, then waits for the guide star to settle back to the new reference before the imager takes the next exposure.

INDIGO Guider Agent provides three dithering strategies:

* **Random Spiral** *(default)*
* **Random**
* **Spiral**

The strategy controls **where** the next dither position is placed. The settle detection algorithm is the same for all three strategies.

Dithering is orthogonal to the drift correction and detection modes described in [INDIGO_GUIDER_CORRECTION_MODES.md](INDIGO_GUIDER_CORRECTION_MODES.md) and [INDIGO_GUIDER_DETECTION_MODES.md](INDIGO_GUIDER_DETECTION_MODES.md). It adds a controlled offset to the guiding reference point; the correction modes then guide the telescope to that new reference point as they would guide to any target.

### Why dither?

Without dithering, the same patch of sky falls onto the exact same pixels in every frame. This makes the stacked result vulnerable to:

* **Fixed-pattern noise** — Pixel-to-pixel response variations, column or row defects, and amp glow are mapped identically onto the same position in every frame. They cannot be removed by averaging alone.
* **Hot pixels and bad columns** — A hot pixel always appears at the same position in every frame and survives stacking unless dithering moves the target off it.
* **Walking noise** — Low-spatial-frequency noise that repeats at the same sky position across many frames correlates into the stack.
* **Drizzle under-sampling** — Drizzle needs different sub-pixel sampling positions across input frames. Without dithering, all frames contribute only from the same sub-pixel position and the reconstructed image is not improved.
* **Read noise patterns** — Certain CMOS sensors exhibit fixed read-noise patterns (e.g., horizontal banding). Dithering shifts those bands to different sky positions and they partially average out in the stack.

---

## How Dithering is Triggered

The guider agent owns the dithering mechanics — the strategy, the offset calculation, and the settle detection. However, **dithering is triggered by the imager agent**, not by the guider agent itself.

The imager agent decides after each captured frame whether to request a dither from the guider, based on the following parameters found in the imager agent **Batch** settings:

* **Frames to skip before dither** — Controls how many frames are captured between successive dither moves. Range: −1 to 1000. Default: 0.

  | Value | Behaviour |
  |-------|-----------|
  | `−1` | Dithering disabled entirely |
  | `0` | Dither after every frame |
  | `N > 0` | Capture N frames, then dither; repeat — i.e., dither every $(N+1)$-th frame |

  The imager tracks the countdown in real time in the **Frames to dithering** statistics item so you can see how many frames remain before the next dither.

* **Dither after last frame** (feature switch) — When enabled, the imager also triggers a dither after the very last exposure in a batch. Useful when a subsequent batch should start from a fresh dither position.

* **Enable dithering** (feature switch) — Global enable for the imager-side dithering. When disabled, no dither requests are sent to the guider regardless of the other settings.

### Choosing the Skip Count

The optimal skip count depends on how long each exposure is relative to the guider's settle time:

| Exposure time | Settle time | Typical skip count | Notes |
|---------------|-------------|-------------------|-------|
| Short (< 30 s) | Longer than the exposure | 1–3 | Dithering after every frame would dominate imaging time |
| Medium (1–5 min) | Much shorter than the exposure | 0–1 | Settle overhead is small; dither freely |
| Long (> 5 min) | Negligible fraction of exposure | 0 | Dither after every frame costs almost nothing |

A higher skip count reduces the fraction of total imaging time spent waiting for settle, at the cost of fewer distinct dither positions in the stack.

---

## Strategy 1: Random Spiral

*This is the default strategy.*

### How It Works

Random Spiral visits positions on an outward-growing spiral, but adds a small random perturbation to each computed spiral point:

```math
x_k = x_{\text{spiral},k} - d_x \cdot U_x, \qquad
y_k = y_{\text{spiral},k} - d_y \cdot U_y
```

where $(x_{\text{spiral},k}, y_{\text{spiral},k})$ is the deterministic spiral position for the $k$-th dither, $(d_x, d_y)$ are the direction signs for the current quadrant, and $U_x, U_y \sim \mathrm{Uniform}(0, 1/1.1)$.

This gives:

* a broad, systematically growing spatial coverage — as with the plain Spiral —
* plus enough randomness to avoid any strict periodicity or grid alignment.

Over a long imaging session the positions are well spread across the full dither area without large clusters or large gaps.

### When to Use

* **General-purpose dithering** — This is the best default for most sessions.
* **Drizzle** — Provides good sub-pixel sampling coverage with no repeating pattern.
* **Fixed-pattern and walking noise rejection** — Non-repeating positions prevent the noise from correlating back into the stack.
* **Long imaging sessions** — The growing spiral ensures positions are systematically spread over the whole dither area.

### When Not to Use

* If you need a perfectly deterministic, reproducible sequence — use **Spiral**.
* If you need pure statistical randomness without any spiral tendency — use **Random**.

---

## Strategy 2: Random

### How It Works

Each dither offset is drawn independently and uniformly:

```math
x_k \sim \mathrm{Uniform}\!\left(-A, +A\right), \qquad
y_k \sim \mathrm{Uniform}\!\left(-A, +A\right)
```

where $A$ is the **Dithering max amount (px)**. There is no memory of previous positions — each dither is independent of the last.

### When to Use

* **Short sessions where spiral coverage is unnecessary** — A few frames dithered randomly performs about as well as Random Spiral.
* **Hot pixel and bad column rejection** — A shift of even 1–2 px is sufficient; pure random is ideal.
* **Read noise pattern rejection** — Random shifts break detector banding patterns reliably.
* **When you want no structured spatial bias** — Pure random has no preferred direction or quadrant.

### When Not to Use

* **Very long sessions** — Pure random has no coverage guarantee. A few percent of frames may cluster near already-visited positions, slightly wasting dither moves.
* **Drizzle with many frames** — Spiral or Random Spiral provide better sub-pixel sampling coverage guarantee.

---

## Strategy 3: Spiral

### How It Works

Spiral visits a fixed, deterministic sequence of positions. The pattern cycles through four quadrants and spirals outward monotonically:

```math
x_k = d_x \cdot \left( \lfloor k/4 \rfloor \bmod \lfloor A/2 \rfloor + 1 \right), \qquad
y_k = d_y \cdot \left( \lfloor k/4 \rfloor \bmod \lfloor A/2 \rfloor + 1 \right)
```

where $d_x, d_y \in \{-1, +1\}$ cycle through the four quadrant sign combinations as $k \bmod 4$ changes. The sequence never repeats within one full spiral revolution and covers positions systematically from the center outward.

### When to Use

* **Repeatable sessions** — If you want the same dither sequence every session (e.g., for systematic testing or comparison between nights).
* **Drizzle with a fixed number of frames** — When the exact number of frames is known in advance, the Spiral guarantees uniform quadrant coverage.
* **Careful systematic sky coverage** — The deterministic pattern is easy to reason about and verify.

### When Not to Use

* **Sessions where exact reproducibility is not needed** — Random Spiral is usually a better default.
* **Very long sessions beyond one full spiral revolution** — The pattern repeats exactly, which reintroduces positional correlation.

---

## Comparison Summary

| Strategy | Coverage guarantee | Randomness | Main use case |
|----------|--------------------|------------|---------------|
| **Random Spiral** | Good — grows outward systematically | Moderate — jittered positions | General purpose; drizzle; long sessions |
| **Random** | Statistical only | Full | Short sessions; hot pixel / banding rejection |
| **Spiral** | Deterministic, uniform per revolution | None | Reproducible sequences; fixed-count drizzle |

---

## Settle Detection

After the telescope is moved to the new dither position, the guider must bring the guide star back to that new reference and confirm the image has stabilized before signalling the imager to take the next frame.

INDIGO Guider Agent settle detection works as follows:

1. **At dither start**, the current RA and Dec RMSE (computed over the previous guiding frames) is captured. The settle thresholds are set to:

```math
\sigma_{\text{RA,thr}} = 1.5 \cdot \sigma_{\text{RA}} + \frac{e_{\min}}{2}, \qquad
\sigma_{\text{Dec,thr}} = 1.5 \cdot \sigma_{\text{Dec}} + \frac{e_{\min}}{2}
```

where $\sigma_{\text{RA}}, \sigma_{\text{Dec}}$ are the pre-dither RMSE values and $e_{\min}$ is **Min error**.

2. **During settling**, RMSE is accumulated starting from zero over the post-dither frames.

3. **Settle is declared** when both conditions hold:
   * at least **Dithering settling limit (frames)** frames have been accumulated, *and*
   * the accumulated RMSE (RA and/or Dec as appropriate) is below the threshold captured in step 1.

4. **Settle timeout**: If settle has not been declared within **Dithering settle time limit (s)**, the settle attempt is declared failed and an error message is sent to the client. The imager may choose to abort or skip the frame.

If the pre-dither RMSE was zero (e.g., first dither of the session), no RMSE threshold is applied and settle is declared purely on the frame count.

---

## Parameters

* **Dithering max amount (px)** — Maximum offset from the reference in pixels. The actual applied offset depends on the strategy (see above). Range: 0–15 px. Default: 1 px.
* **Dithering settle time limit (s)** — Maximum time allowed for the guide star to settle after a dither before reporting a failure. Range: 0–300 s. Default: 60 s.
* **Dithering settling limit (frames)** — Minimum number of guide frames that must be accumulated post-dither before settle can be declared, regardless of RMSE. Range: 1–50 frames. Default: 5 frames.

---

## Choosing the Dither Amount

The right amount depends on what you are trying to achieve:

| Goal | Minimum amount | Typical amount | Notes |
|------|---------------|----------------|-------|
| Hot pixel / bad pixel rejection | 1 px | 1–2 px | Even 1 px is sufficient to move a point source off a single bad pixel |
| Read noise pattern / banding rejection | 2 px | 2–5 px | More shift breaks the pattern more effectively |
| Fixed-pattern noise and amp glow | 3 px | 3–7 px | The larger the feature, the larger the shift needed |
| Drizzle sub-pixel sampling | 1 px | 2–5 px | Need enough variety of sub-pixel positions; more frames compensate for smaller amount |
| Bad column / row rejection | equal to column width | 5–10 px | Shift must exceed the defect width |

Larger amounts increase the dither hop and require more settle time. If guiding is marginally stable, prefer smaller amounts (1–3 px) with a larger settling limit.

---

## Effect on Stacked Images

### Drizzle

Drizzle combines undersampled frames by placing each input pixel's flux at its specific sky coordinate on a finer output grid. Without dithering, all input frames contribute from identical sub-pixel offsets and drizzle cannot recover any additional resolution. With dithering, each frame comes from a different sub-pixel position. The more varied the positions, the better the reconstructed resolution.

* **Random Spiral** or **Random** with amount 2–5 px typically provides good sub-pixel coverage over 10–20 frames.
* Very small amounts (0.5 px or less in the main camera equivalent) barely help drizzle.
* Very large amounts increase effective PSF motion and require more frames to fill the output grid.

### Read Noise / Fixed Pattern Noise

Dithering is particularly effective on CMOS sensors that exhibit:

* **Horizontal or vertical banding** — Periodic read noise at fixed rows or columns. After dithering, a given sky position falls on a different row/column in each frame. Sigma-clipping or simple averaging then rejects or attenuates the band.
* **Amp glow / gradient** — A fixed illumination gradient from the readout amplifier. Dithering shifts the gradient relative to the sky and median-combining the registered frames attenuates it.
* **Salt-and-pepper hot pixels** — Any shift ≥ 1 px moves these to a different sky position in each frame and they are trivially rejected by sigma-clipping or minimum-rejection combining.

### Walking Noise

Walking noise (also called correlated noise or low-frequency noise) refers to noise whose power spectrum is concentrated at low spatial frequencies. Without dithering, the same low-frequency noise pattern repeats at the same sky position in every frame and adds coherently. With dithering, the noise pattern shifts relative to the sky between frames and is partially averaged out.

---

## Typical Starting Configurations

### General multi-frame imaging session

* **Strategy:** Random Spiral
* **Amount:** 2–3 px
* **Settling limit:** 5 frames
* **Settle time limit:** 60 s

### Drizzle session with 20+ frames

* **Strategy:** Random Spiral or Random
* **Amount:** 3–5 px
* **Settling limit:** 5–8 frames
* **Settle time limit:** 60–90 s

### Short session, mostly hot pixel rejection

* **Strategy:** Random
* **Amount:** 1–2 px
* **Settling limit:** 3–5 frames
* **Settle time limit:** 30–60 s

### Sensor with strong banding noise

* **Strategy:** Random or Random Spiral
* **Amount:** 5–10 px
* **Settling limit:** 5–8 frames
* **Settle time limit:** 60–120 s

### Reproducible test session

* **Strategy:** Spiral
* **Amount:** 3–5 px
* **Settling limit:** 5 frames
* **Settle time limit:** 60 s

---

## See Also

* [INDIGO_GUIDER_CORRECTION_MODES.md](INDIGO_GUIDER_CORRECTION_MODES.md) — how the guider converts drift into correction pulses.
* [INDIGO_GUIDER_DETECTION_MODES.md](INDIGO_GUIDER_DETECTION_MODES.md) — how drift is measured from the guide frames.

---

Clear skies!
