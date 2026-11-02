# Specification

AC-1: named DIMLEDS Error returns false and preserves reason, even if generic cmd says done. AC-2: documented named DIMLEDS done and legacy generic cmd done remain accepted; missing/malformed ACK fails. AC-3: ALTO/GIOTTO/focuser LED property remains old value and ALERT after refusal; healthy command/reconnect remains supported.

Baseline b6d3ee192, serialized shared helper consumed by three generated drivers. Existing final full hardware-free runs retained in previous task; this repair uses relevant baseline and negative regression before production changes. Device protocol fixtures document generic cmd acknowledgement for GIOTTO1.03.09 and SESTOSENSO3, named DIMLEDS for older focuser controllers. No public properties changed. Medium complexity/functional risk. Hardware decision: none. All builds isolated in /private/tmp/indigo-led-ack, host macOS arm64, other platforms unavailable.
