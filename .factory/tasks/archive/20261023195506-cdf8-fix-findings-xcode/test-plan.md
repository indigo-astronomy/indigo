# Test plan

Use disposable registry/project fixtures to exercise archival, creation, rollback, wrong placement, missing references, ID preservation, unsupported structure, collision, symlinks, idempotence and repositories without Xcode. Run the full Factory suite and configured static/security checks. Independently parse the real project with plutil and compare active/archive navigator paths with filesystem inventories; compare unrelated objects against master.
