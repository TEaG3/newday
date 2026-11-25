# Stage live media refresh without mutating the active catalog

Introduce a live media-monitoring path that builds a complete candidate catalog away from the active UI data, computes a stable-key diff, and only publishes the candidate at a quiescent render-frame boundary. The work must preserve the current catalog if scanning fails, is cancelled, or produces invalid/duplicate records.

The first implementation step is deliberately isolated: add a portable snapshot/diff core and a reader/apply gate with host concurrency tests. Do not call load_games() from a monitor thread and do not enable periodic scanning until every active cv[] reader and every media-I/O worker participates in the guard.
