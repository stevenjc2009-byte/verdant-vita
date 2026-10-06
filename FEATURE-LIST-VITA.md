# Verdant Vita: requested changes and next features

## Implemented for 0.3.3

| Feature | Behavior |
|---|---|
| More accurate keyboard without resizing | A finger highlights a key, slides to correct it and commits on release. Small movements across a boundary retain the highlighted key; sliding off the keys cancels. Joystick clicks still type immediately. Key sizes and layout are unchanged. |
| Proper cursor | White arrow with a black outline and its click point at the tip. Cursor-only damage tracking and restoration are retained. |
| Notepad File menu | New, Open, Save and Save As. Open/Save As currently ask for a full path through the keyboard; a graphical file picker is a future feature. |
| Notepad Edit menu | Select All, Copy, Cut and Paste; select-all highlighting and replacement. The clipboard can hold the whole supported small document. Arbitrary range selection and undo/redo are future features. |
| Cleaner Start menu | Terminal, Files, Notepad, Calculator and Games stay on the desktop and are omitted from Start. Other applications remain in Start. |
| Games folder | Snake, Falling Blocks and Brick Breaker run natively, with touch/D-pad controls, pause/restart and saved high scores. Desktop Games opens the launcher. `.vgame` launch files also live in `ux0:/verdant/games/` and can be opened from Files. These are game launcher markers, not Linux executables. |
| Understandable CPU charts | Physical core charts identify UI, I/O, Linux and display/system roles. The page states that Linux has one virtual CPU assigned to Core 2. |

These are lightweight native Vita applications in Verdant. They do not require Linux to run their game logic. Doom is not included in this build.

## Why the core loads are uneven

Your 8%, 3%, 81%, 10% readings are consistent with one heavy Linux-emulation worker and lighter native workers. The native UI is on Core 0, native file/HTTPS/audio work uses Core 1, the single RV32 Linux CPU is interpreted on Core 2, and display transfer tries Core 3 when unlocked. If the affinity request fails it uses Core 1. The charts include Vita system activity, so Core 3's 10% alone does not establish whether CapUnlocker succeeded; the page's selected-core status is the better indicator.

Work is already spread where tasks can execute independently. The largest remaining task is a single stream of emulated CPU instructions. Several threads cannot safely run the next instruction of that same virtual CPU at once. Changing affinity to move it between physical cores would distribute the *reported location* of the load without increasing the amount of work completed in parallel. Making every chart equal is therefore not a useful speed target.

[CapUnlocker](https://github.com/GrapheneCt/CapUnlocker) permits homebrew to use the fourth physical core. It does not provide an emulator with extra virtual CPUs. Replacing the interpreter with a carefully developed dynamic recompiler, or developing native ARM Linux support, could tackle the actual bottleneck; both are substantial engineering projects. This release does not claim to have implemented them or to have fixed the specific 81% plateau.

## Recommended next features

1. **Touch calibration:** a short target-tapping setup to measure a consistent finger offset, stored per device; optional click sound or vibration feedback where supported.
2. **Keyboard editing:** hold Backspace to repeat, selectable repeat delay, and a clearly marked press/release preview. Terminal Ctrl/Alt behavior must remain intact.
3. **Notepad file picker:** graphical Open/Save As with folders, filename entry and overwrite confirmation.
4. **Notepad editing:** arbitrary text selection, undo/redo, Find/Replace, line numbers, font controls and unsaved-change confirmation for New/Open/Close.
5. **Game options:** difficulty, sound, controller mapping, touch-drag paddle control and visible high-score tables.
6. **More small native games:** Minesweeper, Pong, Solitaire, 2048 and a simple maze game. Native implementations avoid adding interpreter work to Core 2.
7. **Doom/Freedoom integration:** investigate a native engine rather than putting an RV32 game onto the already busy Linux CPU. A [PrBoom-Plus Vita port](https://github.com/fgsfdsfgs/prboom-plus) already documents Doom and Freedoom data support. Its existing VPK is a separate application, not a window in Verdant. Integrating it into Verdant needs graphics, audio, controls and engine lifecycle work. Original Doom requires game data; [Freedoom](https://freedoom.github.io/about.html) supplies free alternative game content and still needs an engine.
8. **Worker diagnostics:** show actual worker names, selected affinity and thread CPU time if exposed. This would distinguish emulator work from display/file/network work more clearly than aggregate core charts.
9. **Guest activity trace:** record which Linux jobs and services run during high Core 2 load, with low overhead, to diagnose the exact plateau on the physical Vita.
10. **Idle and I/O profiling:** measure polling, virtual disk/9p calls, timer interrupts and interpreter instruction costs on-device before making further scheduling changes.
11. **Background queue improvements:** prioritize short interactive operations ahead of bulk copies/downloads, while retaining cancellation and verified installation.
12. **Interpreter acceleration study:** benchmark a dynamic recompiler and its ARM code-generation/memory-coherency requirements. A multi-vCPU guest is a separate emulator/kernel change, not an affinity setting.

The implemented changes have host regression coverage and a VitaSDK build. Hardware touch accuracy, game performance and post-update core utilization still need testing on the Vita. No general desktop scaling or unrelated theme/layout redesign is part of this release.
