# Tethering and macOS transport investigation

The following primary sources were inspected on 2026-10-26:

- [pkTriggerCord](https://pktriggercord.melda.info/) explicitly lists K-m/K2000 with full settings, capture and RAM download support. Supported systems are Linux, Windows, OpenBSD and Android; macOS is absent. Its connection guidance requires MSC rather than PTP. This establishes camera protocol feasibility, not a working transport on this Mac.
- [Ricoh IMAGE Transmitter 2](https://www.ricoh-imaging.co.jp/english/products/imagetransmitter2/) supports selected newer cameras, not K-m.
- [Tether Studio](https://tetherstudio.app/cameras) advertises K-m USB support while describing a PTP engine. No inspected implementation establishes its MSC transport. The owner subsequently tried it and reported that it could neither detect nor connect to this K-m. Its compatibility list is therefore not accepted as evidence of working support on this setup. No purchase or third-party contact was performed by the agent.
- [Apple SCSITaskUserClientIniter](https://github.com/apple-oss-distributions/IOSCSIArchitectureModelFamily/blob/main/UserClient/SCSITaskUserClientIniter.cpp) disables SCSI task user-client property merging for disk peripheral type 00. The current K-m registry has that type and the initer category, but lacks the required task device properties. SDK and [XNU disk ioctl definitions](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/disk.h) provide no generic CDB passthrough. No supported native SCSI path was confirmed, and no user client or kernel property was forced.
- [Apple capture entitlement](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.vm.device-access), [Apple DTS](https://developer.apple.com/forums/thread/747029) and [libusb FAQ](https://github.com/libusb/libusb/wiki/FAQ) explain capture requirements. No VM entitlement was added, respecting the owner's App Store constraint.

## Local evidence

Ordinary execution, a temporary copy signed with the standard USB entitlement, the older installed libusb, UID 0 execution, and UID 0 combined with the standard USB entitlement all failed interface claim with -99 before camera vendor commands or exposures. Elevated debug output confirms UID 0; insufficient privilege alone therefore does not explain the remaining failure.

A privileged attempt detached the system Mass Storage driver before claim failed. A temporary utility using public libusb_attach_kernel_driver restored it successfully, and a class-only registry check confirmed the original binding. Product code now explicitly pairs detach/claim/release/reattach, including failed-claim rollback. The utility remains outside the repository and sent no vendor commands.

IORegistry retains a USB user client owned by ChatGPT; libusb also warned that another process held exclusive access. This is a concrete candidate conflict, not proof of the sole cause. No app was stopped or injected. The next meaningful physical test should run independently after the holding app is closed, with the camera awake. Repeating tests under unchanged ownership would waste batteries.
