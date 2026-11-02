# Specification

AC-1: Partial HTTP requestline/headerprogress and aggregateframing have finite monotonic deadlines; client slots released. GET/PUT callback APIs and body ownership preserved. HTTP header block budget4KiB, existing perline budget,64headers. Defaults5sprogress/30stotal, testoverrides0.2/1s. Established XML/JSON/websocket traffic is not subjected to HTTPframing deadlines.
AC-2: Blank/whitespace ACL lines cannot indexbeforebuffer; comments and normaldevice/master records retain behavior.

Base9557ca85e. Highrisknetwork trustboundary. Existingblobuploadline/body bounds5s/60s preserved. GenericPUT handlers read their ownheaders, so server preflight peeks completebounded header beforedispatch withoutconsumingcallbackbytes; no publicABI or globalI/O semantics change. No externalnetwork/hardware.
