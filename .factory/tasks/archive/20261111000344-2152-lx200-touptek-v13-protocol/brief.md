# Task brief

- Task ID: 20261111000344-2152-lx200-touptek-v13-protocol
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-11

## Problem and desired outcome
The ToupTek SH20 profile of `indigo_mount_lx200` was written from ToupTek's INDI driver and the TMount ASCOM server, before a protocol document existed. The owner supplied the official ATLAS SH20 Developer Protocol V1.3 (`ATLAS_Command_Protocol_V1.3_EN.pdf`, 2026-10-30, firmware 0.5.2.20261029).

An analysis of the driver against V1.3 listed what V1.3 has, what the driver is missing and what it does wrong. The owner asked: "fix the wrong points".

## Users and use cases
Users of an ATLAS / ToupTek SH20 mount on V1.3 firmware:
- goto and sync
- guiding
- park and unpark
- home
- site setup
- meridian flip settings

## Scope and non-goals
In scope are the nine "wrong" points of the analysis:
1. `:MS#` reply polarity.
2. `:Sgr0.n#` format.
3. Fixed `+00` meridian offset.
4. Meaning of the meridian `b` flag.
5. Whole-second site.
6. Remove `:MFH#` home search.
7. `:GHS#` home completion.
8. Unpark preconditions and confirmation.
9. Outdated comments and documentation on the `e` code and `N/A#` replies.

The changes cover the simulator, the tests and the documentation.

Non-goals are the "missing" V1.3 features:
- SMeq/SMMC
- tracking after goto
- the Td/readback sequence around Sr/Sd
- SDST0
- UTC offset minutes
- GES
- GRl/GRV/RV and SRl for the SH20
- altitude limits
- the mode switch
- MAz
- GmV
- the abort follow-up
- one axis at a time

## Constraints and references
- The `.driver` file is the source of truth; regenerate the `.c`.
- No README change without approval.
- No commits before owner approval.
- No push.
- V1.3 PDF: `/home/rumen/Downloads/ATLAS_Command_Protocol_V1.3_EN.pdf`. It is not added to the repository.
- Existing protocol notes: `indigo_drivers/mount_lx200/PROTOCOL.md`, ToupTek SH20 section.

## Initial questions
Should firmware before V1.3 (`:MS#` → `0#` as success) stay supported? It was asked in the analysis and not answered. The owner's "fix the wrong points" is read as following V1.3; the `e` code tolerance stays (see specification).
