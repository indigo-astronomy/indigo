# Test plan

Use same adapter, different cable and mount; any changed polarity cannot alone identify which changed component caused it. Read PMC8 raw positions/rates/directions, convert signed 24-bit counts with documented model scale and confirmed physical home/hemisphere. Hold transport open during motion/restoration because serial DTR closure can reset PMC8 counters. Use idle and repeated direction controls, bounded moves/pulses and explicit stop checks; no claim from host timer alone.
