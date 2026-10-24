# Task brief

Owner: Peter Polakovic. Created: 2026-10-24.

Fix FND-20261024183351-e8e1-fix-motion-image-alpaca-001: the initial Lunatico poll must not overwrite an accepted focuser motion request before its queued handler runs. Scope: both Lunatico exports, their shared implementation/simulator, existing driver tests and required Factory evidence. No generator/framework changes, new properties, hardware access, merge or push are authorized by this task.
