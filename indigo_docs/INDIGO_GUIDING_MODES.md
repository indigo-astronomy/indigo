# INDIGO Guider Agent - Guiding Modes

Revision: 31.03.2026 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Overview

INDIGO Guider Agent provides four drift-correction modes. They can be selected independently for the Right Ascension and Declination axes.

* **RA** supports: **Proportional-Integral**, **Hysteresis**, and **Linear Trend**.
* **Dec** supports: **Proportional-Integral**, **Hysteresis**, **Linear Trend**, and **Resist Switch**.

Each mode converts the measured guide-star drift into a correction pulse in a different way. No single mode is always best. The best choice depends on the mount mechanics, guide exposure, seeing, backlash, and the shape of the tracking error.

As a rule:

* Start with **PI**.
* Switch only when you have a clear reason.
* Choose by **measured mount behavior**, not only by the mount's advertised drive type.

## Mode 1: Proportional-Integral (PI)

*Available for: RA and Dec. **This is the default mode.***

### How It Works

The PI controller is described in detail in [GUIDING_PI_CONTROLLER_TUNING.md](GUIDING_PI_CONTROLLER_TUNING.md). In brief:

* The **Proportional** term (P) reacts to the current measured drift. It corrects a fraction of the present error immediately.
* The **Integral** term (I) uses the recent average drift history to add correction for residual systematic error that the P term did not fully remove.

In simplified form:

```math
correction = -\left(P \cdot \text{drift} + I \cdot \text{avg\_drift} \cdot \text{guide\_cycle}\right)
```

The P term is fast. The I term is slower, but it helps with steady trends such as residual periodic error or slow Dec drift.

### Parameters

* **RA Proportional aggressiveness (%)** and **Dec Proportional aggressiveness (%)** — Fraction of the measured drift corrected immediately. Default: 80%.
* **RA Integral gain** and **Dec Integral gain** — Strength of the integral term. Setting either one to 0 disables the I term for that axis. Default: 0.5.
* **Integral stack size (frames)** — Number of frames used to build the drift average for the I term. If this is set to 1, the controller behaves as pure P regardless of the gain values. Default: 1.

### When to Use

* **General-purpose guiding** — This is the best starting point for most mounts.
* **Moderate periodic error** — Especially in RA, a small I term can help remove slow residual PE.
* **Slow monotonic drift** — Useful for gentle Dec drift from minor polar misalignment or refraction.
* **Well-characterized systems** — PI is the easiest mode to tune systematically.

### When Not to Use

* **Large Dec backlash with I enabled** — The integral term can accumulate error across reversals and may cause overshoot or oscillation. In such cases use P-only, **Hysteresis**, or **Resist Switch** for Dec.
* **Strongly noisy drift measurements** — If seeing or centroid noise dominates, the I term may slowly integrate noise. Reduce the I gain or disable it.
* **As a substitute for poor polar alignment** — Guiding can hide some drift, but it cannot eliminate field rotation.

---

## Mode 2: Hysteresis

*Available for: RA and Dec.*

### How It Works

The Hysteresis controller blends the current drift with the previous controller output before computing the new correction:

```math
blended = (1 - h) \cdot \text{drift} + h \cdot \text{prev\_output}
```

```math
correction = -\text{aggressiveness} \cdot \text{blended}
```

where $h$ is the **Hysteresis (%)** value expressed as a fraction from 0 to 1.

This acts as a low-pass filter on the correction signal. Short-lived spikes are softened, while persistent drift is still corrected. If the drift falls below **Min error**, no correction is issued and the stored output naturally resets to zero.

### Parameters

* **RA Hysteresis aggressiveness (%)** and **Dec Hysteresis aggressiveness (%)** — Overall correction gain. Default: 70%.
* **RA Hysteresis (%)** and **Dec Hysteresis (%)** — How much of the previous output is blended into the new correction. 0% means no memory. 100% means maximum memory. Default: 10%.

### When to Use

* **Noisy centroid measurements** — Good when seeing, short exposures, or camera noise make the drift jump around.
* **Mounts that guide well but the graph looks noisy** — Hysteresis often calms the guide graph without over-complicating tuning.
* **Dec guiding with modest backlash** — It adds memory without the windup risk of an integral term.

### When Not to Use

* **Strong systematic drift** — The smoothing delays the response. PI usually handles real drift better.
* **Fast disturbances** — Wind gusts or rapid PE need a quicker controller.
* **Excessive hysteresis values** — High values can make the response sluggish and can leave the controller lagging behind the real error.

---

## Mode 3: Linear Trend

*Available for: RA and Dec.*

### How It Works

The Linear Trend controller keeps a short history of recent drift measurements and fits an Ordinary Least Squares (OLS) line through them. The slope of that line estimates how quickly the drift is changing.

For a populated history, the internal correction is approximately:

```math
correction = -\text{slope} \cdot N \cdot \text{aggressiveness}
```

where $N$ is the number of samples currently stored in the trend history.

In other words, the controller tries to follow the **trend**, not only the current error. It can therefore anticipate smooth drift better than a purely reactive controller.

Important implementation details:

* With fewer than four samples in history, the mode behaves like a simple proportional controller.
* If the drift is below **Min error**, no correction is issued, but the history is kept.
* If the current drift is a large outlier (more than about four times **Min error**), the history is cleared and the controller falls back to a proportional-style response for that sample.
* If the calculated trend correction is larger than the observed drift, the controller falls back to a proportional-style response and repeated rejections eventually clear the history.
* If the calculated trend would push in the wrong direction, the correction is suppressed.

### Parameters

* **RA Linear Trend aggressiveness (%)** and **Dec Linear Trend aggressiveness (%)** — Overall gain applied to the trend estimate. Default: 80%.

### When to Use

* **Smooth monotonic drift** — For example mild Dec drift, refraction-related drift, or very smooth long-period PE.
* **Predictable slow error** — Works best when the drift evolves approximately linearly over several guide samples.
* **Cases where PI with I becomes unstable** — Linear Trend can be cleaner than a strong integral term when the error is smooth but the system is sensitive.

### When Not to Use

* **Poor seeing or noisy guiding data** — The slope estimate becomes unreliable.
* **Backlash-dominated Dec behavior** — The trend assumption breaks down when reversals are absorbed by backlash.
* **Fast or irregular PE** — Rough, asymmetric, or high-frequency error is usually a poor fit for this mode.
* **Very short guide exposures** — The history fills with atmospheric noise rather than real mount behavior.

---

## Mode 4: Resist Switch

*Available for: **Dec only**.*

### How It Works

Resist Switch is designed specifically for backlash-prone Dec guiding. It is inspired by the approach used in PHD2.

The controller keeps a short history of recent Dec drifts and forms **direction votes** from samples that exceed **Min error**. Once it establishes a preferred correction side, it tries hard to stay on that side. It will switch only when there is enough evidence that:

1. the star is consistently drifting to the opposite side, and
2. the drift is getting worse rather than improving.

This prevents unnecessary North/South reversals that would repeatedly take up backlash.

An optional **fast-switch threshold** can override the normal conservatism. If the drift suddenly becomes large in the opposite direction, the controller can force a quick side re-evaluation.

### Parameters

* **Dec Resist Switch aggressiveness (%)** — Overall correction gain. Default: 100%.
* **Dec Resist Switch fast-switch threshold (px)** — If non-zero, a sufficiently large opposite-direction drift forces a fast re-evaluation of the preferred correction side. Set to 0 to disable. Default: 0.6 px.

### When to Use

* **Substantial Dec backlash** — This is the main use case.
* **One-direction Dec guiding** — Especially useful with *North only* or *South only* Dec guiding.
* **Well-polar-aligned systems with infrequent Dec corrections** — It avoids unnecessary reversals and keeps the mount loaded in one direction.

### When Not to Use

* **RA guiding** — It is not available for RA and would be inappropriate there.
* **Tight, low-backlash mounts** — On these mounts it is usually too conservative.
* **Large continuous Dec drift** — If polar alignment is poor, a more direct controller is usually better.
* **Wildly alternating seeing excursions** — Repeated large sign changes can prevent timely correction.

---

## Comparison Summary

| Mode | Axes | Usually best for | Usually not ideal for |
|------|------|------------------|-----------------------|
| **Proportional-Integral** | RA, Dec | General-purpose guiding; moderate PE; slow systematic drift | Dec backlash with a strong I term; very noisy guide data |
| **Hysteresis** | RA, Dec | Noisy centroids; smoother response; good general-purpose alternative when guiding is noisy; moderate backlash sensitivity | Fast real drift; rapid disturbances; high-PE slopes |
| **Linear Trend** | RA, Dec | Smooth monotonic drift; long, gentle trends | Rough PE; turbulent seeing; backlash-dominated Dec |
| **Resist Switch** | Dec | Significant Dec backlash; conservative one-side Dec guiding | Low-backlash systems; continuous Dec drift; RA |

---

## Mount Type Guidance

The table below gives **typical** recommendations. It is intentionally conservative. Real mounts vary widely, and the guiding graph should always overrule the drive label.

| Mount / drive type | Typical behavior | Usually suitable modes | Usually not the first choice | Notes |
|--------------------|------------------|------------------------|------------------------------|-------|
| **Worm gear** | Often smooth RA periodic error; Dec backlash is common; behavior is usually predictable | **RA:** PI, sometimes Linear Trend if PE is smooth and guide cadence is short enough. **Dec:** PI, Hysteresis, or Resist Switch if backlash is significant | **Dec:** Linear Trend on backlash-heavy mounts; Hysteresis with high $h$ when clear systematic drift is present | This is the most common case. If the mount is mechanically sound, PI is usually the best starting point on both axes |
| **Strain wave / harmonic drive** | Usually little classical backlash, but RA error can be rough, asymmetric, and steep; some mounts show elasticity or high-frequency components | **RA:** PI, often P-only or with a small I term; Hysteresis can also help if the RA trace is dominated by high-frequency jitter or centroid noise. **Dec:** PI or Hysteresis. | Linear Trend is often a poor fit for rough or jagged RA error. Resist Switch is usually unnecessary unless Dec backlash is actually observed | Use short enough guide exposures to sample the steeper RA error. If the high-frequency oscillation is real mount motion rather than guide noise, PI is usually safer than Hysteresis. Choose by measured behavior, not by the harmonic label alone |
| **Friction drive / roller drive** | Very low backlash and little classical gear PE, but slip, stiction, or wind sensitivity may appear | **RA/Dec:** PI or Hysteresis; Linear Trend if the drift is smooth and monotonic | Resist Switch unless there is real Dec reversal deadband | These mounts often do not need backlash-specific strategies, but may benefit from a calmer controller if the centroid is noisy |
| **Direct drive** | Very low backlash; no worm PE, but servo jitter, encoder noise, or external disturbances may dominate | **RA/Dec:** PI or Hysteresis | Resist Switch in most cases; Linear Trend as a default | If the mount already tracks very smoothly, Hysteresis can reduce chasing tiny noise. Use PI if there is real low-frequency drift |
| **Belt-reduced / hybrid gear trains** | Behavior depends strongly on what the belt drives; backlash can be low, but compliance and irregular error may appear | Usually PI first, then Hysteresis if the guide data is noisy | Linear Trend if the error is irregular; Resist Switch unless Dec backlash is confirmed | Treat these mounts by their measured guide behavior, not by the presence of a belt alone |

### Practical interpretation

* **If the mount has obvious Dec backlash**, prefer **Resist Switch** or sometimes **Hysteresis** for Dec.
* **If the mount has smooth, slow error**, **PI** or **Linear Trend** can work well.
* **If the mount has rough or jagged RA error**, **PI** is usually the safest first choice. **Hysteresis** can also work well when part of the roughness is guide-star noise or short-term jitter, but **Linear Trend** is usually a poor fit.
* **If the graph is mostly noisy rather than drifting**, **Hysteresis** is often the right mode to try.

---

## Typical Starting Configurations

### General use (well-polar-aligned mount, no obvious special issues)

* **RA:** Proportional-Integral, aggressiveness 90%, Integral gain 0, Integral stack size 1
* **Dec:** Proportional-Integral, aggressiveness 90%, Integral gain 0, Integral stack size 1

Refer to [GUIDING_PI_CONTROLLER_TUNING.md](GUIDING_PI_CONTROLLER_TUNING.md) for the full tuning procedure.

### Mount with noticeable Dec backlash

* **RA:** Proportional-Integral, P-only, aggressiveness about 70-80%
* **Dec:** Resist Switch, aggressiveness 90-100%, fast-switch threshold 0.6 px

### Turbulent seeing or noisy guide star centroid

* **RA:** Hysteresis, aggressiveness 70%, Hysteresis 10-15%
* **Dec:** Hysteresis, aggressiveness 70%, Hysteresis 10-15%

### Smooth but measurable Dec drift

* **RA:** Proportional-Integral, P-only, aggressiveness about 70-80%
* **Dec:** Linear Trend, aggressiveness 80%

### Typical harmonic / strain-wave mount starting point

* **RA:** Proportional-Integral, usually P-only first, aggressiveness about 70-80%; **Hysteresis** is also a reasonable starting choice if the RA graph is dominated by noise or short-term jitter
* **Dec:** Proportional-Integral or Hysteresis depending on noise level

### Typical low-backlash direct-drive or friction-drive starting point

* **RA:** Proportional-Integral or Hysteresis
* **Dec:** Proportional-Integral or Hysteresis

Avoid **Resist Switch** on these mounts unless you have measured real Dec reversal deadband.

---

Clear skies!
