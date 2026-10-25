# Specification

AC-1: Replace maintained navigator synchronization rules with exclusion of .factory and descendants from Xcode references/build membership. Preserve INDIGO-specific owner rules and all metadata/history.
AC-2: Findings create/archive/check never read or write Xcode projects; retire sync-project and obsolete helper while preserving record validation, approvals, duplicate checks and transaction rollback. Keep provenance pin unchanged; this is a narrowly reviewed local customization, not a whole template upgrade.
AC-3: Remove only the Factory navigator subtree from maintained indigo.xcodeproj; prove parsed application/build object semantics unchanged. Do not mutate vendored libusb Xcode project, native source, driver/hardware tests or the dirty original checkout.
AC-4: Run configured Factory checks/tests/security, plist/semantic/preservation checks, independent parent-arranged review and committed-range process validation. Produce a ready local commit only. Owner explicitly defers the full audit only for this change, then it remains due without checkpoint reset.

Risk medium for record transaction integrity and project metadata cleanup. Complexity medium: explicit dependency removal and object-graph preservation; no application behavior change or hardware activity.
