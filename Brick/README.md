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

The target firmware sysroot remains outside Git. Configure these repository
Actions secrets before running the workflow:

- `BRICK_SYSROOT_URL` (required): a protected HTTPS download URL for a `.tar`,
  `.tar.gz` or `.tar.zst` sysroot archive.
- `BRICK_SYSROOT_SHA256` (recommended): the lowercase SHA-256 digest of that
  archive.

The archive must contain `usr/include` at its root or below one top-level
directory. It must provide the same SDL2, SDL2_image, SDL2_ttf, ALSA and C/C++
runtime files as the tested Brick firmware. Do not commit the archive, its URL
or access credentials.

Successful runs publish `PegasusG-Brick-<version>.zip` and its `.sha256` file
as a workflow artifact retained for 30 days. Download the artifact from the
completed Actions run; only promote it to a GitHub Release after completing the
device checklist in `STABLE_BASELINE.md`.

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
