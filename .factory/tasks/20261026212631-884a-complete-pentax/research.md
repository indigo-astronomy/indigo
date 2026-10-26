# Pentax protocol sources and implementation approach

Read-only research by independent in-host analyst, October26 2026. User requested all available relevant open-source sources; this records those inspected, not a claim of exhaustive internet coverage.

- Local libgphoto2 camlibs/pentax, revision bceecb0f0365d015b564e4b08122e28eb16088db: pslr.c buffer/select/download/shutter/program paths, pslr_model.c K-m parser, pslr.h exposure enum. Upstream https://github.com/gphoto/libgphoto2/tree/master/camlibs/pentax.
- Local indi-3rdparty/indi-pentax and libpktriggercord, revision fa36bff53dabf16533aed5dac3e688b2f80e692f: INDI capture and CLI bulb behavior. Upstream https://github.com/indilib/indi-3rdparty/tree/master/indi-pentax.
- https://github.com/asalamon74/pktriggercord and its pentax_scsi_protocol.md: command framing/segment/download evidence. Source headers LGPL3+; INDI wrapper LGPL2.1+. OpenPENTAX permissive source provenance was not located. No implementation copied or new library dependency introduced; code written from observed protocol facts and existing INDIGO APIs.

K-m/K2000: BE status412bytes, RAM mask0x1a, shutter numerator/denominator0x30/34, program native0xb0, imageformat0x7c (JPEG0/RAW1/RAW+2), res0x80, quality0x84 (0..2), rawformat0x88 (PEF0/DNG1). Native M8/B9 set through command GUI6/7. WU1->setting->WU2. Trigger button05 value2; old-bulb button0d value1 then trigger then value0. No reliable timed mechanical abort found; suppress output and drain only own resulting RAM frame.

Select args(buf,type,res,0) group02 command01 length16; typePEF0/DNG1/JPEG2+quality. Segment group04 command00 returns16bytes [unused,type,address,length]; type1marker/4output offset/3data/2end. Advance every descriptor via group04 command01 arg0,100mssettle,status. Download blocks<=65536 args(address,length),group06 command00,status,directSCSI f0 24 06 02 00 00 00 00,status. Delete own RAM buffer only via group02 command03 argbuf. Preserve initial mask and card images. Bounded implementation validates contiguous offsets, no address/size overflow, <=4segments/10descriptors/64MiB.
