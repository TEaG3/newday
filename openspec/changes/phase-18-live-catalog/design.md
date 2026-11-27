# Design

## Current hazard

launcher.c has one global cv[MAXC] array. load_games() writes records in place and assigns ncv only at the end. The render thread reads cv throughout a frame; cover_thread, icon_thread, the disc thread, achievements, and launch paths also read records. cover_lock protects cover pointers only; it does not protect catalog metadata or the other readers. The disc drive already demonstrates the safer pattern for one record: stage disc_stage, then copy cv[0] on the render thread between frames.

Therefore a monitor thread must never call load_games() against the active cv[].

## Snapshot and identity

- Build a complete candidate snapshot in a separate buffer. A failed or cancelled scan discards the candidate and leaves the active snapshot unchanged.
- Stable identity is (source_type, source_unit, kind, relative_path). Do not use the current sorted index or title as identity.
- Content changes are represented separately by size, fingerprint, and flags. The scanner must derive a fingerprint from validated metadata; it must not claim cryptographic identity when it only has size/mtime.
- Validate capacity, safe relative paths, and duplicate keys before computing a diff.
- The diff reports added, removed, and changed old/new indices. Reordering alone is not a content change. The caller can restore selection by stable key.

## Reader/apply gate

catalog_guard is a portable state machine whose lock/unlock callbacks must be backed by a real PS2 semaphore in the target build (pthread mutex in host tests).

- Readers acquire a lease before retaining or dereferencing a catalog record and release it immediately after the last access.
- Requesting an apply is refused while gameplay or storage I/O is active. If either starts after a request but before the swap, `catalog_guard_set_activity()` cancels the pending apply and reopens the reader gate; the caller retains the candidate and retries at a later idle point. This prevents a stale apply request from blocking a new launch or reader.
- An accepted apply closes the reader gate to new readers. The candidate may be swapped only after existing readers drain, scanning has ended, the game is not running, and storage/network I/O is idle.
- The render thread is the only owner allowed to perform the short pointer/count swap, between completed frames. The apply lease remains held until the swap is complete.
- Starting a game or marking storage I/O busy cancels an active scan; the scanner checks the cancellation flag between directories and closes all open handles before discarding its candidate.
- A pending apply can wait while a game is running; it must not block launch or change the visible catalog during gameplay.

## Bundle preparation

The bundle publisher may run a caller-supplied prepare callback after the apply gate is acquired and all active readers have drained, but before any active/candidate pointer moves. The callback may populate only candidate-side state (for example, migrate per-game sidecars by stable key). If preparation fails, the active bundle remains untouched and the apply gate is released; the candidate may be discarded or rebuilt. The callback must not mutate or free active resources. Ownership retirement remains the caller's responsibility after a successful swap.

## Sidecar ownership

The typed sidecar set groups VMC directory/count/state, save-icon pointer/state, cover request/completion/LRU state, and achievement state. Candidate migration copies state only for stable keys whose content metadata is unchanged; transient cover requests are reset. Queued icon, VMC, achievements and achievements-list indices are generation-relative too: they live in the sidecar set and are rebased only when the stable key and content metadata are unchanged. Removed or changed requests are cleared, and the achievements-list cache follows its rebased `ra_list_for` key. Pointer fields are borrowed into the candidate while the old bundle remains active. Only after a successful bundle swap may retirement clear transferred pointers in the old set and invoke the appropriate destructor for removed or changed resources. A failed/unpublished candidate is discarded by clearing borrowed pointers without freeing active resources. Missing destructors or stale pointers outside the old snapshot fail closed before ownership is changed.

## Monitoring policy

- Poll only sources whose mount state can be safely queried. Use a conservative 3–5 second interval after device-specific behavior is verified; do not assume a PS2SDK hotplug event API exists.
- Scan only when the console is idle and the media-I/O gate is clear. Pause catalog scans, cover downloads, and nonessential network work while a game is launching or running.
- Source disappearance or read errors do not mean that user data should be deleted. Preserve the last known-good snapshot and report the source as unavailable until a complete replacement scan succeeds.
- Preserve per-game settings, cover cache, achievements, queued worker requests, and the selected item by stable identity. The current integration rebases worker request indices; the render-thread selection still requires its own stable-key restoration when the apply path is wired. Never reset user state because an entry moved in sort order.

## Activation gates

This change does not activate monitoring. Integration is blocked until all catalog readers in launcher.c are leased, all media workers publish I/O activity, and target-specific lock semantics are verified. Required tests include concurrent readers during apply, cancellation during each source scan, source removal/reinsert, duplicate/path rejection, selection restoration, and real PS2 tests for USB, HDD/APA, SMB, and MMCE.
