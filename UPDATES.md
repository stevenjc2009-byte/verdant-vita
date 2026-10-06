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
