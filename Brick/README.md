# TrimUI Brick port

## Stable baseline

The default files in `Brick/launcher/` correspond to
`1.1.1-hotfix3-brick-stable-rollback`. This is the last baseline confirmed to
enter PegasusG successfully while retaining Brick autostart.

Stable behavior:

- AArch64 PegasusG frontend.
- Firmware-provided RetroArch and GBA cores.
- GBA, GBA hack and GBA vib collections.
- 1024x768 display configuration.
- Menu-controlled autostart through `System/starts` and `/tmp/cmd_to_run.sh`.
- MainUI handoff through SIGKILL because Brick MainUI ignores SIGTERM.

## Build

Provide an AArch64 sysroot containing SDL2, SDL2_image, SDL2_ttf and ALSA.
The sysroot is intentionally excluded from Git.

```sh
PEGASUSG_SYSROOT=/path/to/brick-sysroot \
PEGASUSG_OUTPUT=Zip \
./Brick/build_app.sh
```

On Windows with WSL:

```powershell
.\Brick\build_app.ps1 -Sysroot 'D:\path\to\brick-sysroot' -Output Zip
```

Output is created under `Brick/dist/`.

## GitHub Actions package build

`.github/workflows/brick-build.yml` builds an installable AArch64 ZIP after a
push to `dev/brick` that changes `src/`, `Brick/` or the workflow itself. It can
also be started manually from **Actions > Brick package > Run workflow**, with
an optional package version.

The job runs in a Debian Bullseye container and installs the AArch64 SDL2,
SDL2_image, SDL2_ttf and ALSA development packages there. No repository secret
or committed sysroot is required. This produces a low-glibc AArch64 CI package,
but it does not replace testing against the target firmware: verify the ZIP on
the Brick before promoting it to a stable release.

Successful runs publish two workflow artifacts retained for 30 days:

- `PegasusG-Brick-<version>-Full`: a clean-install package with the application
  entry, stable launchers, UI assets, 11 built-in tracks, configuration and
  notices. It deliberately excludes FFmpeg, experimental cores and an enabled
  autostart hook.
- `PegasusG-Brick-<version>-Update`: only the frontend binary, stable launchers
  and version file, for updating an existing complete installation.

Only promote a package to a GitHub Release after completing the device
checklist in `STABLE_BASELINE.md`.

## Experimental work

`experimental/hotfix4-launcher/` contains the encoding-safe ZIP launcher and
custom AArch64 gpSP route. `experimental/gpsp-aarch64/` contains the pinned
upstream revision, patch and build procedure.

These files are retained for investigation only. Known results:

- Normal AArch64 gpSP launched successfully.
- ZIP startup was slow because every launch extracted and deleted the ROM.
- BusyBox rendered Chinese member names as `?`; extension-based extraction fixed it.
- Forced-rumble gpSP froze at the first rendered frame on the test device.
- The combined hotfix was rolled back after PegasusG stopped entering normally.

Do not copy experimental files into a stable package without completing the
checklist in `STABLE_BASELINE.md`.
