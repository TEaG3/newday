# Live catalog refresh

## Requirements

- A scan MUST build a complete candidate snapshot without mutating the active catalog.
- A candidate MUST be rejected on unsafe paths, duplicate stable keys, capacity overflow, or incomplete scans.
- Stable identity MUST use source identity, entry kind, and source-relative path; title and sorted index are not identities.
- An apply MUST wait until all catalog readers have released their leases, scanning has stopped, storage I/O is idle, and no game is running.
- The active catalog MUST be swapped only at a render-thread safe point.
- A scan MUST be cancellable when gameplay starts or storage/network activity becomes busy.
- A failed or cancelled scan MUST preserve the last known-good catalog and all user data.
- The monitor MUST remain disabled until all cv[] readers and media workers participate in the guard and target concurrency tests pass.

## Current status

The portable snapshot/diff and guard core is implemented and host-tested. The PS2 launcher does not yet use it; live monitoring remains disabled.
