# INDIGO Imager Agent - Multi-camera Synchronization

Revision: 01.04.2026 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Overview

INDIGO Imager Agent can synchronize image acquisition across multiple Imager Agent instances. This is intended for dual-camera and multi-camera setups where several cameras should:

* start each exposure at the same point in the batch,
* wait for each other at defined execution points,
* optionally dither in a coordinated way,
* or be released by an external script or operator.

The synchronization mechanism is built from three properties:
* **`AGENT_IMAGER_BREAKPOINT`** — defines **where** the batch may pause,
* **`AGENT_IMAGER_RESUME_CONDITION`** — defines **how** it is released,
* **`FILTER_RELATED_AGENT_LIST`** — defines **which other agents** participate.

Internally, synchronization is implemented by pausing the batch on selected breakpoints through **`AGENT_PAUSE_PROCESS`**. In barrier mode, one Imager Agent acts as the coordinator and monitors the pause state of the related Imager Agents through **`AGENT_IMAGER_BARRIER_STATE`**.

In practice:

* Use **Barrier** on exactly one Imager Agent.
* Use **Trigger/manual** on all follower Imager Agents.
* Select the follower Imager Agents as **related agents** on the coordinator.
* For synchronized dithering, connect the **Guider Agent** only to the coordinator.

As a rule:

* Start with **Pre-capture** if you only need simultaneous exposure start.
* Add **Post-capture** when you want all cameras to finish a frame before the next common step.
* Let only one Imager Agent handle dithering and inter-frame delay.
* Avoid circular related-agent graphs.

## Quick Decision Guide

| Goal | Coordinator | Followers | Breakpoints | Notes |
|------|-------------|-----------|-------------|-------|
| Start all exposures together | **Barrier** | **Trigger/manual** | **Pre-capture** | Best default starting point |
| Keep all cameras aligned frame-by-frame | **Barrier** | **Trigger/manual** | **Pre-capture**, optionally **Post-capture** | Useful when cameras should move through the batch in lockstep |
| Synchronize dithering | **Barrier** | **Trigger/manual** | **Pre-capture** + **Post-capture** | Connect the **Guider Agent** only to the coordinator |
| Wait for an external event or script | Optional | Optional | Any needed breakpoint | Use **Trigger/manual** and release externally |
| Fully independent cameras | Disabled | Disabled | None | Leave synchronization disabled |

## Resume Condition 1: Trigger/manual

*Useful for: follower cameras, script-controlled workflows, manual release.*

### How It Works

When a selected breakpoint is reached, the Imager Agent sets **`AGENT_PAUSE_PROCESS`** busy and waits.

In **Trigger/manual** mode the agent does **not** try to release itself. It remains paused until one of the following happens:

1. the process is aborted,
2. a client or script clears **`AGENT_PAUSE_PROCESS`**, or
3. another coordinating Imager Agent releases it as part of a barrier.

This mode is therefore the passive building block used by follower cameras.

**Trigger/manual is the default resume condition.** It is pre-selected when an Imager Agent is first started.

### When to Use

* **Follower cameras** in a synchronized multi-camera setup.
* **Script-driven release** — wait for weather, temperature, mount state, or other external conditions.
* **Manual step-by-step debugging** of the batch flow.
* **Asymmetric workflows** — e.g. one camera takes many short frames while another waits for a single long frame boundary.

### When Not to Use

* **When one agent should automatically release all others** — use **Barrier** on the coordinator instead.
* **When no breakpoint is enabled** — this mode has no effect by itself.

---

## Resume Condition 2: Barrier

*Useful for: one coordinator Imager Agent controlling a synchronized group.*

### How It Works

In **Barrier** mode one Imager Agent becomes the coordinator.

When the coordinator reaches a selected breakpoint:

1. it pauses itself,
2. it watches the pause state of all related Imager Agents through **`AGENT_IMAGER_BARRIER_STATE`**,
3. once **all related Imager Agents are paused**, it clears **`AGENT_PAUSE_PROCESS`** on every follower,
4. then it resumes its own batch as well.

The coordinator also propagates its **`AGENT_IMAGER_BREAKPOINT`** selection to the related Imager Agents and forces those related Imager Agents to use **Trigger/manual**. This is why only one Imager Agent should use **Barrier**.

When a synchronized batch is started from the coordinator, it also starts exposure batches on the related Imager Agents automatically. If the coordinator is aborted, it aborts the related Imager Agents too.

### When to Use

* **True multi-camera synchronization** with one camera acting as the leader.
* **Simultaneous exposure start** across multiple imaging trains.
* **Coordinated dithering** where one camera controls the cadence and the guider.
* **One-button operation** — start and stop the whole synchronized group from one Imager Agent.

### When Not to Use

* **On more than one Imager Agent in the same group** — this can deadlock.
* **In circular related-agent chains** — recursion and deadlock are possible.
* **If each camera should run fully independently** — leave synchronization disabled.

---

## Available Breakpoints

The batch can pause at any combination of the following stages:

| Breakpoint | Meaning | Typical use |
|------------|---------|-------------|
| **Pre-batch** | Before the first frame of the batch | Synchronize whole-batch start |
| **Pre-capture** | Immediately before each exposure starts | Simultaneous shutter start |
| **Post-capture** | Immediately after each exposure ends | Wait until all cameras have finished the frame |
| **Pre-delay** | Before the inter-frame delay starts | Coordinate the post-capture transition — **coordinator only** (followers skip the delay section entirely) |
| **Post-delay** | After the inter-frame delay ends | Synchronize the next cycle after waiting — **coordinator only** (followers skip the delay section entirely) |
| **Post-batch** | After the final frame of the batch | Synchronize batch completion |

The most useful breakpoint for ordinary multi-camera imaging is usually **Pre-capture**.

## Coordinator vs Follower Behavior

A synchronized setup works best when the roles are explicit:

### Coordinator

The coordinator is the only Imager Agent configured with:

* related follower Imager Agents selected in **`FILTER_RELATED_AGENT_LIST`**,
* **`AGENT_IMAGER_RESUME_CONDITION = BARRIER`**,
* optional related **Guider Agent** for dithering.

The coordinator is responsible for:

* starting the follower batches,
* releasing the barrier,
* performing dithering,
* applying the inter-frame delay,
* aborting the synchronized group if needed.

### Followers

Followers are the other Imager Agents participating in the synchronized batch.

They should:

* use the breakpoints imposed by the coordinator,
* use **`AGENT_IMAGER_RESUME_CONDITION = Trigger/manual`**.

In the code path used for synchronized control, any Imager Agent with Resume Condition = Trigger/manual and at least one breakpoint enabled is treated as a **controlled instance**. After each frame it immediately waits at the breakpoint and **automatically skips dithering and the inter-frame delay** — these are performed only by the coordinator. This is enforced by the code, not just a guideline.

## Configuration Recipe 1: Synchronize Exposure Start

Use this when several cameras should begin each exposure together.

### Coordinator Imager Agent

* Select the other Imager Agents in **`FILTER_RELATED_AGENT_LIST`**.
* Enable **`AGENT_IMAGER_BREAKPOINT.PRE_CAPTURE`**.
* Set **`AGENT_IMAGER_RESUME_CONDITION.BARRIER`**.
* Configure the batch as usual.

### Follower Imager Agents

* Do not select related Imager Agents.
* Use **`AGENT_IMAGER_RESUME_CONDITION.TRIGGER`**.
* Use the same batch count as the coordinator.

### Result

All Imager Agents pause at **Pre-capture**. Once the coordinator sees that all followers are paused, it releases them together and all exposures start in sync.

## Configuration Recipe 2: Synchronize Dithering

Use this when several cameras image the same target and should all wait for the same dither event.

### Coordinator Imager Agent

* Select the follower Imager Agents in **`FILTER_RELATED_AGENT_LIST`**.
* Also select the **Guider Agent** as a related agent.
* Enable **`AGENT_IMAGER_BREAKPOINT.PRE_CAPTURE`** and **`AGENT_IMAGER_BREAKPOINT.POST_CAPTURE`**.
* Set **`AGENT_IMAGER_RESUME_CONDITION.BARRIER`**.
* Enable dithering and configure its cadence in the batch settings.

### Follower Imager Agents

* Use **`AGENT_IMAGER_RESUME_CONDITION.TRIGGER`**.
* Do not configure them as the dithering controller.
* Use the same batch count as the coordinator.

### Result

All cameras finish the current frame, wait at the common breakpoint, and only the coordinator triggers the Guider Agent to dither. After the dither completes, the synchronized group proceeds to the next exposure cycle together.

## Other Useful Workflows

The same mechanism can also be used for:

* **waiting for weather clearance** before a batch or before each frame,
* **waiting for cooler temperature** to stabilize,
* **synchronizing a short-exposure stream with a long-exposure reference camera**,
* **operator-controlled step execution** during testing or troubleshooting,
* **script-controlled orchestration** from INDIGO Script.

## Caveats

* **Only one Imager Agent should use Barrier** in a synchronized group.
* **Do not create circular related-agent references**.
* **Use the same batch count** when frame-by-frame synchronization is required.
* **Dithering and the inter-frame delay are automatically suppressed for followers** (any agent with Trigger/manual + a breakpoint enabled skips dithering and delay entirely).
* **Auto-start of follower batches has a known race condition** — when the coordinator starts a batch and immediately launches the follower batches, there is a small window where a follower may not yet be ready. In practice this is rarely a problem, but be aware when designing time-critical configurations.

## Comparison Summary

| Resume condition | Role | Main strength | Main limitation |
|------------------|------|---------------|-----------------|
| **Trigger/manual** | Follower / passive participant | Simple waiting point; easy to release from script or manually | Does not coordinate the group by itself |
| **Barrier** | Coordinator / active leader | Starts, synchronizes, and releases the whole group | Must exist on one agent only; can deadlock if misconfigured |

## Typical Starting Configurations

### Two cameras, simultaneous shutters

* **Coordinator:** Barrier
* **Followers:** Trigger/manual
* **Breakpoint:** Pre-capture

### Two or more cameras with common dithering

* **Coordinator:** Barrier
* **Followers:** Trigger/manual
* **Breakpoints:** Pre-capture + Post-capture
* **Guider Agent:** related only to the coordinator

### Script-driven acquisition gate

* **All participants:** Trigger/manual
* **Breakpoint:** whichever stage should wait
* **Release:** external script or operator

---

## See Also

* [INDIGO Guider Agent - Dithering](INDIGO_GUIDER_DITHERING.md) — how the Imager Agent triggers dithering through the Guider Agent.
* [INDIGO Agents](INDIGO_AGENTS.md) — general concept of related agents.
* [Imager Agent README](../indigo_drivers/agent_imager/README.md) — implementation notes and property-level details.

---

Clear skies!
