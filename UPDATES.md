# GitHub system updater — 0.2.1

Update channels are pinned in the service source. Version 0.2.1 includes the network-transport correction and bounded HTTPS retries validated against the live release channels.

| Console | Repository | Stable release asset |
|---|---|---|
| 3DS / 2DS family | `stevenjc2009-byte/verdant-3ds` | `verdant-3ds-update.zip` |
| PS Vita | `stevenjc2009-byte/verdant-vita` | `verdant-vita-update.zip` |

The Vita is not the original PSP. There is no Verdant PSP release in these channels.

## On the console

Open **Settings → Upd** or **System update** in the launcher. **Check** queries the latest stable GitHub release. **Update** downloads and stages it. **Auto+** enables download/staging of newer releases on each launch; **Auto-** leaves automatic checks only. **Cancel** stops an active job. Job output shows download progress and errors. The startup check runs in a minimized System update window, which can be selected through window switching.

Exit with Start so Linux can sync and power off, then launch again. Before booting Linux, the native updater rechecks every staged SHA-256 and applies program/service/image files. It exits at an update message; launch once more to run the new executable. This extra relaunch prevents mixing a running old executable with a newly replaced runtime.

- 3DSX: the executable and icon are replaced automatically at the standard installed path.
- CIA: the updater invokes the application manager to overwrite **only** title `000400000F912200`. The package identity is checked against the running Verdant title. Actual self-installation has not been tested on hardware; if it fails, use the downloaded `cias/verdant.cia` in FBI, or relaunch to retry.
- Vita: the staged VPK is unpacked into a restricted list of files for **only** `ux0:/app/VRDT00001`. The updater replaces those files on relaunch. Storage access depends on homebrew permissions; this path has not been tested on hardware. If denied, install the downloaded `ux0:/verdant.vpk` in VitaShell.

The updater preserves `rootfs.ext2`, swap, settings, history, keys and documents. Old program files are under `verdant/update-backup/VERSION/`. Interrupted native transactions retry using staged-file hashes and already-installed hashes. Hold **B** when launching to bypass an update and enter the recovery console. Failed downloads/staging never become a pending native transaction; incomplete staging directories are retained for diagnosis and can be removed while the app is closed.

## Verification and publication

HTTPS certificate and hostname validation use the bundled Mozilla CA certificates from [curl](https://curl.se/docs/caextract.html). No GitHub login/token is stored on the console. Redirects are restricted to GitHub's release hosts. Draft/prerelease releases, malformed semantic versions, wrong-console packages, unexpected archive paths, duplicate entries, oversized packages and mismatched hashes are rejected. The GitHub asset digest and the package's per-file manifest are checked. This trusts the GitHub repository/account and TLS; it is not an independent release-signing system.

Future releases must use a stable `vMAJOR.MINOR.PATCH` tag. Update `VU_VERSION` in `source/core/updater.h`, `VERSION` in `guest/verdant-updater.py` and the package version in `tools/package_release.py` together, build the console executable, then package the matching platform. Upload the fixed-name ZIP to that platform's GitHub release. GitHub's automatic repository source ZIP is **not** an install/update asset. Publish a draft only after its required assets are uploaded. See [GitHub releases API](https://docs.github.com/en/rest/releases/releases).

The first 0.1 builds have only the offline updater. Install 0.2.0 once using FBI/Homebrew Launcher or VitaShell to gain the built-in GitHub updater. Thereafter the application can check and stage future stable releases itself.

## Tests and limits

Automated tests cover platform/hash rejection, HTTPS host restrictions, archive traversal, release staging, native interrupted-rename recovery and preservation of user data. Console Wi-Fi, slow guest HTTPS, title installation and power-loss behavior on physical SD cards remain unverified. A repository with no newer published release correctly offers no update; an updater does not create new versions by itself.

## Vita 0.3.0

Verified HTTPS is performed by a native worker on application core 1, with 20-second connection, 30-second stalled-transfer and bounded job timeouts. Progress/errors appear in the updater. The guest retains GitHub SHA-256, archive and platform validation. Each redirect is restricted to GitHub release hosts; TLS certificate and hostname checks stay enabled. Existing 0.2.6 updaters can stage the same ZIP format, then apply it on relaunch. An old already-running updater cannot receive the new transport until updated; if it stays stuck, install the full release VPK once in VitaShell.

## Vita 0.3.1 native checks

The Vita System update panel now has **Check now**, **Download**, **Auto: on/off** and **Cancel** buttons. Check now bypasses the RV32 Linux guest/Python entirely and starts verified native HTTPS immediately. It times out after 45 seconds, displays errors and can be cancelled. Strict stable-release metadata and numeric versions are checked before showing an update. A fresh verified release response is cached for up to five minutes and reused when staging. Download still requires the Linux service for SHA-256 and archive/platform checks; that phase displays preparation, byte progress, verification and unpacking status. There is no fixed speed guarantee on a physical Vita. The old installed updater remains unchanged until 0.3.1 is applied; a full VPK installation is the fallback.

Version 0.3.1 also fixes a guest-service crash caused by sorting the native `host-http.req` filename as a numeric desktop request. Native HTTP mailboxes and desktop requests now remain separate.

## New in Vita 0.3.2

Vita finger input targets application buttons directly, independently of the joystick cursor. Drag/resize also uses finger coordinates. The optional relative pad works on empty desktop space; app controls, launcher and keyboard stay direct touch. The cursor hides during a touch. The left stick has one-third of its former maximum speed and a quadratic precision curve near center.

The Terminal toolbar now has a **Keyboard** button. Its replacement keyboard uses a larger QWERTY layout, wide Shift/Enter/Space/Backspace keys, numbers, punctuation, Ctrl/Alt/Escape and navigation keys, without function keys. New terminals start Bash directly with a small startup file, preserved history and user `.bashrc`, rather than automatically running neofetch. `neofetch` and `bash -l` remain available manually.

Task Manager puts process names on the left, then CPU %, RAM MiB and PID, with an explanation derived from the actual program and script. Per-process GPU/network throughput is labeled unavailable. Core 2 runs the one-vCPU RV32 interpreter; the native desktop is C code, while Python handles Linux bridge/background services. Physical core percentages and guest process CPU percentages measure different work.

The display worker tries CPU 3 (CapUnlocker) and falls back to CPU 1 when denied. Task Manager reports its selected core. The CPU clock policy preserves clocks at 444 MHz or higher, including a user-selected 500 MHz clock. It does not force an unsupported overclock. Independent native work uses multiple physical cores; the guest remains one virtual CPU.

Supported ux0 file listing, small text open/save, mkdir, file copy and move/rename run on an asynchronous native worker. Linux-only paths, directory copy, search/trash and other complex operations still use the guest. Document saves retain the previous file before committing because Vita rename is not an atomic replacement. An interrupted `.verdant-save-backup` or `.verdant-save-part` remains recoverable.

Idle half-second redraws are removed. Covered windows and unchanged job output do not trigger unnecessary painting. Cursor-only transfer/scaling uses damage regions across both alternating buffers. Graph samples and scale maps are cached. Quiet guest services poll at 10 Hz rather than keeping every idle terminal at 50 Hz. Native HTTPS retains DNS/TLS/connection caches. Networking starts without a 15-second Wi-Fi association wait.

The release retains the full compatible `verdant-vita-update.zip`. An installed 0.3.1 updater uses that package once. Once 0.3.2 is running, later same-runtime releases can use `verdant-vita-fast-update.zip` and reuse Image only after native hash validation, avoiding another roughly 56 MB runtime download. The standalone VPK remains self-contained.

VitaSDK builds and sanitized host/regression tests pass. The actual RV32 Linux guest starts an interactive fast terminal and returns process roles on the host fixture. The prompt appeared in 0.414 seconds on that PC fixture, not a measured Vita time. Physical FPS, touch accuracy, post-update CPU load and CapUnlocker behavior still require console testing.
