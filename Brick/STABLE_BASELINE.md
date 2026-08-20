# Brick stable baseline and device checklist

## Confirmed environment

- Userspace: AArch64.
- Dynamic loader: `/lib/ld-linux-aarch64.so.1`.
- No `/usr/lib32` or `/lib32`; H700 ARM32 cores cannot run on this firmware.
- MainUI ignores SIGTERM; the stock wrapper accepts exit status 137.

## Stable files

- `Brick/launcher/launch.sh`
- `Brick/launcher/autostart_ctl.sh`
- `Brick/launcher/autostart_launch.sh`
- Frontend built with `PEGASUSG_BRICK`.

## Before promoting a Brick release

1. Launch PegasusG from the official menu.
2. Exit PegasusG and confirm return to MainUI.
3. Enable autostart, power off through the official menu, and cold boot.
4. Repeat after a long-press power-off test.
5. Disable autostart and confirm the next boot stays in MainUI.
6. Scan GBA, GBA hack and GBA vib with Chinese and ASCII filenames.
7. Launch an uncompressed `.gba` with every supported stable core.
8. Test ZIP only if the stable launcher explicitly supports it.
9. Verify saves, resume, brightness, volume and input mappings.
10. Keep `/mnt/SDCARD/Apps/PegasusG/data/brick-port.log` for failures.

## Current rollback point

If an experiment prevents app entry, restore the three stable launcher scripts
and a frontend binary built from the stable source. Experimental cores and
tools may remain on the card because the stable launcher does not reference them.
