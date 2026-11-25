## Stage 1 — portable safety core

- [x] Add bounded snapshot validation with safe relative paths and duplicate stable-key rejection.
- [x] Add stable-key add/remove/change diff that ignores ordering-only changes.
- [x] Add reader leases, scan cancellation, and a quiescent apply gate with caller-supplied locking.
- [x] Add host tests including a real concurrent reader held across an apply request.
- [x] Cross-compile the inactive core with the PS2 EE compiler and enforce an 8 KiB maximum stack frame.
- [ ] Wire the guard to PS2 semaphore callbacks and prove target compiler/runtime behavior.
- [ ] Convert every cv[] reader and every storage/network worker to short-lived leases/activity reporting.
- [ ] Build the inactive candidate catalog without touching cv[]; close handles and discard it on cancellation/error.
- [ ] Apply the pointer/count swap only on the render thread between frames; preserve selection and per-game state by stable key.
- [ ] Add source polling with cancellation and source-specific mount checks; never assume hotplug notifications.
- [ ] Add target concurrency, USB removal/reinsert, HDD/APA, SMB and MMCE tests before enabling the monitor.
- [ ] Keep the feature disabled until all target and hardware gates pass.
