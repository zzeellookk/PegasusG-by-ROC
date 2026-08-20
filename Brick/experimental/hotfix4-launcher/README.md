# Experimental hotfix4 launcher

This directory preserves the withdrawn custom-core launcher for diagnosis.

- `launch.sh`: routes gpSP to app-local AArch64 cores and extracts ZIP content.
- `extract_gba_zip.sh`: requires exactly one `.gba` or `.agb` member and works
  around BusyBox Chinese names displayed as question marks.
- `launch_gpsp_aarch64.sh`: starts an app-local core through Brick `ra64.trimui`.

Do not use these files in the stable package yet. The full experiment caused a
frontend-entry regression on the test installation, repeated ZIP extraction
made startup slow, and the forced-rumble core froze at the first frame.
