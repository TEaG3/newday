## Stage 1 — portable safety core

- [x] Add bounded snapshot validation with safe relative paths and duplicate stable-key rejection.
- [x] Add stable-key add/remove/change diff that ignores ordering-only changes.
- [x] Capture the boot-time catalogue from every enabled source adapter into a validated stable-key baseline; use source type/unit and an OPL UL-name CRC key rather than the `ul.cfg` row number. Live refresh remains disabled.
- [x] Add a bounded old-to-new index map by stable key; changed entries retain identity mapping but remain distinguishable by size/metadata diff for sidecar reset decisions.
- [x] Add a sidecar migration primitive that preserves state only for unchanged stable keys and zeroes changed/new entries without touching output on validation errors.
- [x] Add reader leases, scan cancellation, and a quiescent apply gate with caller-supplied locking.
- [x] Add host tests including a real concurrent reader held across an apply request.
- [x] Cross-compile the inactive core with the PS2 EE compiler and enforce an 8 KiB maximum stack frame.
- [x] Add the PS2 semaphore adapter with fail-closed `WaitSema`/`DeleteSema` handling; compile it with the PS2 EE compiler and host-test lock failures plus concurrent readers.
- [ ] Verify semaphore lifecycle and concurrent-reader behavior on a real PS2 target.
- [x] Gate cover-cache reads, OPL ART import, cover downloads, update checks, and achievements/VMC work through a shared semaphore; refuse IOP hand-off until tracked workers are idle.
- [x] Gate the icon worker's memory-card installation, setup writes, local VMC reads, and save-icon I/O through the same semaphore; start it only after the gate exists.
- [x] Gate disc detection, libcdvd reads, catalog title lookup, and disc-cover I/O; wait for it to finish before IOP hand-off.
- [x] Wire render-frame and background-worker reader leases into the launcher; report worker I/O and IOP/game handoff through the guard activity state.
- [ ] Narrow long-running network-worker leases to per-entry scopes before enabling staged catalog apply.
- [ ] Build the inactive candidate catalog without touching cv[]; close handles and discard it on cancellation/error.
- [ ] Apply the pointer/count swap only on the render thread between frames; preserve selection and per-game state by stable key.
- [ ] Add source polling with cancellation and source-specific mount checks; never assume hotplug notifications.
- [ ] Add target concurrency, USB removal/reinsert, HDD/APA, SMB and MMCE tests before enabling the monitor.
- [ ] Keep the feature disabled until all target and hardware gates pass.


## Stage 2 — isolated source-scan output

- [x] Refactor ISO/ZSO, UL, HDL, remote-app and local-app source adapters to populate a caller-owned candidate entry buffer instead of writing directly to the active cv[] array.
- [x] Build and validate the initial stable-key snapshot from that candidate, copy the complete initial catalog to cv[], and publish ncv only after the copy; retain a boot-only low-memory fallback before the render stage.
- [ ] Add cancellation checks and guaranteed handle closure to every source adapter, including cooperative cancellation for the remote HTTP app catalogue, before enabling live scans.
- [x] Add optional cancellation callbacks to the local ISO/ZSO, UL, HDD/APA, and APPS candidate scanners; check between file operations and close active image, catalogue, and directory handles before returning.
- [x] Report candidate snapshot readiness/status through caller-owned outputs so a failed or cancelled candidate cannot invalidate the active boot snapshot.
- [x] Fail closed for cancellable refresh while the remote UDPFS app catalogue is enabled; its HTTP transfer has no cooperative cancellation API yet.
- [ ] Migrate all index-keyed sidecars and add end-to-end candidate/apply tests before enabling live monitoring.

- [x] Reject partial HD Loader/APA and USBExtreme UL source scans on directory read, sector read, malformed-length, and close failures; never mark the stable snapshot ready after an incomplete source scan.
