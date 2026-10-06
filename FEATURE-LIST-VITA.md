# Verdant Vita v0.4.0 — changes and Core 2 investigation

## Where to find the changes

| Requested improvement | Implemented behavior |
|---|---|
| Touch calibration | Settings → Touch calibration. Tap five targets. A consistent finger offset is saved in physical pixels and applied to direct touch, the keyboard and game controls. Inconsistent or excessive offsets are rejected. Reset clears the offset; Circle cancels. |
| Slightly larger keyboard | Keys are taller by default. Settings → Keyboard size switches between Larger and Compact without changing global desktop scaling. Touch still highlights, slides to correct, commits on release and cancels off the keys. |
| Proper Notepad editing | Drag text to select; Select All, Copy, Cut, Paste, Undo/Redo, Find, Find Next and Replace All. View has font size and line-number controls. New/Open/Close prompt before discarding changes. Save preserves edits made while an earlier save is pending. Keyboard has its own button. |
| Graphical file picker | Notepad File → Open/Save As browses SD or Linux folders, with selection, parent folder, SD/Root shortcuts and page controls. Name opens the keyboard for a new filename. Save As checks whether the target exists and prompts before replacement. |
| Investigate Core 2 | Task Manager → Diagnostics measures interpreter/trap time, device polling, idle waits, batch/wait rates and a guest program-counter sample. Export report saves `ux0:/verdant/core2-report.txt`, including the available Linux process sample. |
| More informative Task Manager | Linux processes sort by measured CPU use, then RAM, with names and descriptions. Diagnostics show the native UI, Files, interpreter, display and HTTPS threads, their last-used core, waiting/running state and raw kernel clock counter, plus queued file work. Unavailable metrics remain unavailable. |
| Smoother dragging | Settings → Window dragging defaults to Fast outline. The window contents remain in place while an outline follows the pointer; the move/resize commits on release. Border pixels are restored between frames, avoiding full content rendering on every movement. Live contents remains selectable. |
| Responsive background work | Independent short reads/saves can pass queued bulk file operations. Dependencies on the same paths retain their order, and overtakes are limited to avoid starving bulk work. An already running bulk operation is not preempted. The guest scans mailbox names without creating unnecessary Path objects, skips completed background jobs and polls less often when there are no terminals or running jobs. |
| Better game controls | Held directional controls, drag-to-position paddles, pause/restart, visible high scores and speed/difficulty choices for the real-time games. Minesweeper has Flag/Reveal mode. Solitaire uses card/stack selection followed by a destination tap. |
| More games | Seven native games: Snake, Falling Blocks, Brick Breaker, Minesweeper, Pong, 2048 and one-card Klondike Solitaire. Games run inside Verdant without adding a Linux game process. Launcher files appear under `ux0:/verdant/games/`. |
| Clearer update feedback | Elapsed time, average download rate and an approximate remaining time when byte totals are available. Progress animation continues while waiting; stalled transfers are identified after no new output. Failed checks offer Retry. Cancellation can be requested while the Linux service is still accepting a download. |
| Recovery and backups | Backups has Settings export, Restore (tap twice to confirm), Browse and Linux /root archive controls. Exports include preferences, engine and updater settings; copies are verified, and all exported files are validated before restore begins. Existing documents are copied before replacement; companion `.origin` files record their original paths. Settings restore applies on relaunch. Existing interrupted-update recovery and launch-with-Circle boot-console recovery remain available. |

## Core 2: what was found

Linux still has one emulated RV32 CPU, assigned to physical Core 2. Native UI, storage/network and display workers are separate. Parallel native work is useful; moving the single interpreter between cores does not execute its instructions in parallel.

The first host run sampled startup activity and an explicitly requested login shell. The sample reported 100% guest utilization, with shell startup, the desktop service and filesystem initialization contributing. This is not a steady-state Vita measurement.

A later quiet session used the normal fast terminal and waited for startup work to settle. Its guest sample reported about 20.6% utilization, attributed to the Python desktop service, while Bash was idle. A contemporaneous emulator snapshot reported approximately 20.3% wall time in interpreter/trap execution, 0.6% in device polling and 79.1% waiting after guest WFI. These are individual samples on the PC host, not Vita CPU percentages or a performance benchmark. Metrics requests themselves contribute to the service's work.

The code review identified repeated mailbox scans, terminal polling and completed-job checks as work that can be reduced. This release replaces object-heavy request scanning, skips completed jobs and increases the idle interval to 250 ms only when no terminals or jobs are active. Quiet terminals retain a 100 ms interval and active work a 20 ms interval. Native applications remain independent of this polling.

There is no evidence here establishing the exact cause of your physical Vita's 81% plateau. Slower instruction interpretation, guest service activity and physical-system work remain plausible contributors. Use Diagnostics → Export report while the plateau is visible. A dynamic recompiler or multiple virtual CPUs is not implemented in this release, and equal core charts are not a speed target.

## Practical limits

Notepad is a small plain-text editor, with bounded eight-step undo and redo histories. It preserves UTF-8 data, but the current bitmap font does not display every Unicode glyph. Large files should still use Vim or Nano. SD text writes retain the existing 15,000-byte limit. Drag selection uses the visible text; very long lines have no horizontal scrollbar.

Backup exports cover the three named settings files. Previous document copies can be opened through Files and saved back through Notepad. Full Linux disk snapshots require Verdant to be closed. Settings commits are per file: a storage failure during restore can leave a partial restore; the completed export remains available to retry. No live disk-image copying is performed.

Difficulty controls affect the real-time games; puzzle/card games retain their standard rules. Audio, configurable mappings, solitaire undo and game suspension across application restarts are not added. Doom is not bundled and still needs a separate engine integration.

Native thread clock counters are displayed raw; they are not presented as measured CPU percentages. Per-process GPU/network usage and hardware memory type/clock remain unavailable.

## Validation

VitaSDK compilation and executable conversion; sanitized model and actual desktop tests for selection, history, text bounds, menus, picker overwrite checks, calibration, outline restoration and all seven games; native backup validation/recovery and queue-order tests; actual RV32 Linux boot, platform mounts, file operations and process/profiler samples. Installer and old-updater archive compatibility checks are completed before release publication.

Hardware touch accuracy, native thread-counter behavior, game feel and FPS need testing on the physical Vita.
