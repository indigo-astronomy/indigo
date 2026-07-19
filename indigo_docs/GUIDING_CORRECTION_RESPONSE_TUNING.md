# INDIGO Guider Agent - Correction Response Tuning

Revision: 18.07.2026 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Overview

Tuning a guider is usually done by looking at the guiding graph and by watching how the mount reacts to a
simulated bump (the dithering impulse test described in the [PI Controller Tuning](GUIDING_PI_CONTROLLER_TUNING.md)
document). Both methods work well but they require an experienced eye and they tell you very little about what is
happening frame after frame during normal, quiet guiding.

To make this easier the INDIGO Guider Agent publishes two additional numbers in the guiding statistics:
**Correction response RA** and **Correction response Dec** (the `CORR_RESPONSE_RA` and `CORR_RESPONSE_DEC` items,
added to the `AGENT_GUIDER_STATS` property in version 3.0.0.44). They are a compact, quantitative health check of the guiding for each axis,
and they are **independent of the drift correction mode** in use - the same interpretation applies no matter which
correction algorithm you have selected. The idea is simple: guiding that is doing its job should leave behind a
residual error that looks like pure random noise. If the residual still carries a pattern from one frame to the
next, the guiding is either too slow or too aggressive, and the metric will show it.

## What is measured

After each correction the guider knows the *residual* drift of the frame - how far the star still was from the
reference after the previous correction was applied. The correction response is the **lag-1 autocorrelation** of
this residual series, computed separately for RA and Dec.

Lag-1 autocorrelation simply answers the question: *does the residual of one frame tell us anything about the
residual of the next frame?* It is a number between **-1 and +1**:

* If consecutive residuals are unrelated (each frame is an independent surprise), the value is around **0**.
* If a residual tends to be followed by another residual **of the same sign** (the star keeps lagging on the same
  side), the value is **positive**.
* If a residual tends to be followed by one of the **opposite sign** (the star keeps jumping from one side to the
  other), the value is **negative**.

A few practical notes about how it is computed:

* It is evaluated over a sliding window of the last **200** guiding frames and it is **not** published until at
  least **100** frames have been collected. Until then both items read **0**. So give the guider a couple of
  minutes to settle before trusting the numbers.
* The series is **median-centred** and each sample is limited (winsorised) to about 4 robust sigmas, so a single
  bad frame, a cloud, or a lost star cannot flip the sign or dominate the result.
* **Dithering frames are excluded** - a dithering move is an intentional offset, not a tracking error, so it is
  not fed into the metric.
* The value is reported rounded to three decimals. Reading exactly `0.000` for a long time usually means there are
  not yet enough samples, not that the result is perfect.

## How to interpret it

The goal is to get the correction response **close to zero on both axes**. Zero means the residual is dominated by
seeing and random noise - the part of the error that no guider can remove - which is exactly what well tuned
guiding should leave behind.

| Correction response | What it means | What to do |
| ------ | --- | --- |
| **≈ 0**<br>(roughly -0.2 … +0.2) | The residual looks like random noise. The guiding is well balanced for this axis. | Nothing. This is the target. |
| **Clearly positive**<br>(≳ +0.3) | The error **persists in the same direction** frame after frame. The guiding is **under-correcting** or a systematic error (periodic error, polar misalignment, drift) is not being removed. | Make the guiding respond **more strongly / faster** on this axis. If the drift is systematic, address the source (better polar alignment, shorter guiding cycle) or use a correction mode able to remove it. |
| **Clearly negative**<br>(≲ -0.3) | The error **alternates sign** frame after frame - the classic signature of **over-correction / oscillation**. | Make the guiding respond **more gently / slower** on this axis. |

The **sign tells you the direction** of the mistuning (too slow vs. too aggressive) and the **magnitude tells you
how strong** the effect is. A value of +0.6 is a much more sluggish response than +0.3; a value of -0.6 is a much
more violently oscillating one than -0.3. Values near ±1 indicate a badly mistuned axis that needs immediate
attention.

Treat the thresholds above as a guide, not as hard limits. Under mediocre seeing the residual is noisier and the
metric naturally sits closer to zero even when the tuning is not perfect; under excellent seeing small mistunings
become more visible. What matters is the trend as you change the settings.

### Which one is better - a bit positive or a bit negative?

In practice it is hard to keep the correction response sitting exactly on zero - it will wander a little to one
side. If you have to choose, **a small positive value is better than a small negative one**, so aim to leave the
axis just on the positive side of zero (a gentle +0.05 … +0.15) rather than the negative side.

The reason is what each side does to the mount:

* A **slightly positive** value means the guiding is a touch *gentle* - it lets a small error decay over a couple
  of frames instead of erasing it at once. The corrections stay smooth. The only cost is that tiny errors linger a little longer.
* A **slightly negative** value means the guiding *over-reacts* - each correction pushes the star a little past the
  reference, so the next frame has to undo part of the previous move. The star ends up bouncing from one side of
  the target to the other and the guider is fighting its own corrections instead of the real error.
  On the Dec axis these constant direction reversals also run straight into backlash, which tends to make the star
  wander more, not less.

Negative is the side that borders on instability, positive is the side that borders on a calm, stable guiding.

The one caveat: make sure a small positive value is just gentleness on random errors and **not** a real systematic
drift. If the graph shows an obvious one-directional drift or a periodic error at the same time, that positive
number is telling you to fix the source (polar alignment, exposure time, correction mode), not to relax the response.

## Using it to optimise guiding

The correction response does not replace the visual inspection of the graph or the dithering impulse test - it
complements them. The dithering impulse test shows you how the mount reacts to a large, sudden error; the
correction response tells you how well it behaves during long, quiet guiding, which is where most of the imaging
time is actually spent. Because it is a single objective number per axis, it is easy to watch while you change a
setting and see whether things got better or worse.

A convenient way to use it:

1. Get the guiding running and reasonably settled.

2. Let it run quietly for a few minutes so that at least 100-150 frames accumulate and the correction response is
   published for both axes.

3. Read the two numbers:

   * If an axis is **positive**, make its guiding response a little **stronger / faster** and wait for the metric
     to re-stabilise. If it stays positive and the graph shows an obvious drift or periodic error, the problem is
     a systematic error - improve the polar alignment, shorten the guiding cycle, or pick a correction mode that
     can deal with it.

   * If an axis is **negative**, make its guiding response a little **gentler / slower** and wait again.

4. Change **one axis and one setting at a time**, then give it time to re-stabilise before reading the metric
   again - remember it always reflects the last ~200 frames, so it reacts gradually.

5. Repeat until both **Correction response RA** and **Correction response Dec** hover near zero. The value will
   almost certainly drift around a bit, so keep it on the positive side (a gentle +0.05 … +0.15)
   rather than letting it go negative. At that point the guiding is removing everything it reasonably can and the
   remaining error is seeing and noise.

Clear skies!
