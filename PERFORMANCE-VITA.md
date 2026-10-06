# Verdant PS Vita: performance and process report, v0.3.2

Core 2 does most of the Linux work because Verdant assigns its Linux emulator to that physical Vita core. The desktop itself runs as native Vita code. The Linux system runs inside an interpreted RV32 virtual computer with one virtual CPU. This explains the uneven load; it does not identify exactly which process caused your particular 74–75% reading.

## What each core does

| Vita core | Work in 0.3.2 |
|---|---|
| Core 0 | Input, window management and drawing the desktop and native applications. |
| Core 1 | Native HTTPS, storage worker and audio output; display transfer also falls back here if Core 3 is unavailable. |
| Core 2 | Interprets Linux CPU instructions: the Linux kernel, Python services, Bash and all guest Linux programs. It also services the guest's virtual devices. |
| Core 3 | Display transfer/scaling when CPU 3 affinity is permitted by CapUnlocker. Otherwise it remains unavailable to this application's worker. Vita system activity can still appear in its chart. |

The charts measure physical core activity from the Vita kernel's idle-time counters, including system activity. They are not a per-thread breakdown. The process list measures Linux process CPU time inside the virtual machine. Those are different measurements: a Linux process using 10% of the virtual CPU does not necessarily consume 10% of a physical Vita core.

### Why Core 2 stays high

An interpreter translates and executes guest instructions in software. Even small Linux jobs require many native operations. Core 2 therefore handles the kernel, service polling, process creation, shell startup, file access, networking and every Linux application. Other cores can be mostly idle while it works. In the previous version, the automatic `neofetch` welcome display also launched commands to collect system details every time a login shell opened.

The software sleeps when the guest executes its wait-for-interrupt instruction. In 0.3.2 the Python service also polls more slowly when there is no actual activity. A quiet shell no longer keeps it on a permanent 50 Hz polling schedule. Task Manager itself requests Linux metrics every two seconds; gathering those metrics also uses the guest CPU.

I cannot determine the exact cause of the specific 74–75% plateau without a trace from your Vita during that period. It could combine ordinary emulation cost, polling, metric collection, running programs and system activity. It is not evidence that Python alone uses 75%, and it is not a fabricated fixed number in the chart.

### Why the emulator is not spread across four cores

The current emulator models one CPU with a single sequence of instructions and CPU state. Assigning that sequence to several threads would not safely make it four times faster. Multiple cores already perform independent native work. A genuinely multi-CPU guest or native ARM Linux would require a different emulator/kernel and considerable platform work.

The display worker now tries Core 3 and falls back to Core 1 if thread creation or startup fails. Task Manager reports the selected display core. This supports the capability exposed by [CapUnlocker](https://github.com/GrapheneCt/CapUnlocker); actual plugin operation on your console still needs confirmation. The application preserves a CPU clock of 444 MHz or higher, so it does not reduce your selected 500 MHz clock back to 444 MHz. A lower positive reading is raised to 444 MHz. It does not install an overclocking plugin or force unsupported frequencies. [PSVshell](https://github.com/Electry/PSVshell) documents the separate CPU/GPU/bus clock controls. Desktop drawing is currently software rendering, so increasing GPU frequency alone does not accelerate the Linux interpreter.

## What the processes mean

Python is a programming language. A `python3` process is the program that reads and runs a Python script. In Verdant, Python helps the desktop communicate with Linux; it is not the entire operating system or the native desktop renderer.

| Process / script | Plain-language purpose |
|---|---|
| `python3 … verdant-agent.py` | The desktop's Linux helper. It receives commands, starts terminal sessions, carries terminal input/output, manages Linux files and launches background jobs. |
| `python3 … verdant-updater.py` | Checks or stages software updates, examines archives and checks file hashes. Native Vita code performs the HTTPS transfer and final installation. |
| `python3 … verdant-currency.py` | Fetches and parses dated currency rates for Calculator. |
| `python3 … verdant-vnc.py` | Communicates with a remote desktop server. |
| Other `python3` | Another Python program. Task Manager does not invent an identity when it cannot identify the script. |
| `syslogd` | Collects messages that programs send to the system log. It normally spends much of its time waiting for messages. |
| `klogd` | Collects messages from the Linux kernel. |
| `init` — PID 1 in this Linux image | Starts the system's services and cleans up process records when children exit. |
| `bash` | The command interpreter in a terminal: it reads commands and starts programs. |
| `sh` | Another shell, often used to run startup or short command scripts. |
| `dropbear` | The lightweight SSH server or one of its connections. |
| `getty` | Waits for someone to log in on a console. |
| `wget` / `ssh` | A download or an outgoing connection to another computer. |

PIDs 82, 46, 83, 50 and 74 cannot be identified from the numbers alone. The numbers are assigned as processes start and can change after each boot. You no longer remember their names, so this report deliberately leaves those particular identities unknown. The updated process rows show a role derived from the actual process name and script arguments, making the next list easier to understand.

### Why an idle program can use RAM

A program keeps its code, libraries, buffers and data in memory even while it waits. Python additionally needs its interpreter and imported modules, so a Python service can use more RAM than a small logging program. That does not mean it is constantly busy.

The process column reports resident memory, in MiB. Shared library pages may appear in more than one process's resident total, so adding every row is not a reliable measurement of unique physical memory use. The Memory page provides the guest's overall RAM reading separately from the native application heap and Vita memory pools.

Names now appear at the far left, followed by CPU %, RAM and PID. Each row has a short explanation. CPU is unavailable until there are two samples. Reliable per-process network throughput and Vita GPU utilization are not exposed by this implementation, so the UI labels them unavailable. The existing Wi-Fi page shows guest-wide traffic; it does not include native HTTPS or other Vita applications.

## Changes made to reduce delays

- New terminals start interactive Bash directly, using a small startup file and the user's own `.bashrc`. They skip automatic login-profile `neofetch`. Run `neofetch` manually when wanted, or `bash -l` for a login shell. History is still saved.
- Vita Files and Notepad perform supported ux0 directory/text/copy/move operations on a native background worker without waiting for emulated Python. Linux-only paths and more complex operations still use the guest.
- Unchanged desktops no longer redraw on a half-second timer. Hidden windows do not force terminal redraws, and fully covered windows are skipped during painting. Background terminals still execute and keep output.
- Cursor-only changes copy and scale the affected regions instead of the full display. Alternating framebuffers include the preceding damage so they do not leave stale pixels or pointer trails.
- Graphing reuses sampled graph points until the function or viewport changes. Pixel scaling reuses coordinate maps.
- Native HTTPS retains its connection, DNS and TLS caches. Release checks remain independent of Linux startup and retain deadlines, progress, cancellation and verification.
- Jobs are polled per window, so one job does not monopolize a global polling slot. Repeated unchanged job output does not trigger unnecessary repainting.
- Startup initializes networking without waiting 15 seconds for Wi-Fi association. Offline launch can proceed; networking can connect later.
- Future same-runtime updates can use a small ZIP that omits the unchanged Linux Image. The native installer validates the existing Image hash before installing anything. The full update ZIP is retained for older updaters.

Finger taps now target the touched application's buttons independently of the joystick pointer. Touch also drives window drag/resize directly. The optional relative touchpad is limited to empty desktop space; app controls, launcher and keyboard remain direct touch. The cursor is hidden while a finger is down. The stick moves at one-third of its former maximum speed on Vita, with a quadratic curve for finer movement near the center and time-based sensitivity.

The Terminal toolbar now says **Keyboard**. The replacement is a larger QWERTY keyboard with number row, wide Backspace/Tab/Caps/Shift/Enter/Space keys, Ctrl, Alt, Escape, arrows, Home/End, Page Up/Down and Delete. There are no function keys. Shift/Ctrl/Alt apply to the next key; Caps stays selected. It remains a Verdant keyboard so terminal controls and commands work directly, rather than a modal Vita text-entry dialog. Themes and unrelated application layouts were retained.

## Validation and limits

The executable builds with VitaSDK. Sanitized host tests pass for direct-touch calculator actions with the cursor elsewhere, keyboard layout at all five UI scales, editor/files, core selection/fallback, frame ownership, partial transfer/scaling, clock preservation, networking startup, update verification/recovery and service polling.

The actual interpreted RV32 Linux guest booted on the host fixture, started a fast interactive terminal, executed a command, returned process descriptions, accessed Vita storage aliases and completed native HTTPS currency/update jobs. The fast prompt appeared in **0.414 seconds on the PC fixture**. This is a functional architecture check, not a promised Vita startup time. The native Files/Notepad fixture also completed without booting Linux. No console frame rate, physical touch accuracy, Core 3 plugin success or post-update core utilization has been measured here.

Installing 0.3.2 through an installed 0.3.1 updater uses the compatible full ZIP once. After 0.3.2 is running, later releases with the same Linux Image can use the smaller fast-update ZIP. An Image mismatch causes a verified rejection; the full VPK remains the fallback. Existing Linux disks and preferences are preserved.
