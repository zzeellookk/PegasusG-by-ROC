# RG34XXSP physical input map

Captured on the H700 `ANBERNIC-keys` device (`/dev/input/event1`) with
`capture-one.sh` and `evtest --grab`.

| Physical control | Linux event |
|---|---|
| A | `EV_KEY 304 BTN_SOUTH` |
| B | `EV_KEY 305 BTN_EAST` |
| X | `EV_KEY 307 BTN_NORTH` |
| Y | `EV_KEY 306 BTN_C` |
| L1 | `EV_KEY 308 BTN_WEST` |
| R1 | `EV_KEY 309 BTN_Z` |
| L2 | `EV_KEY 314 BTN_SELECT` |
| R2 | `EV_KEY 315 BTN_START` |
| Select | `EV_KEY 310 BTN_TL` |
| Start | `EV_KEY 311 BTN_TR` |
| D-pad up | `EV_ABS 17 ABS_HAT0Y -1` |
| D-pad down | `EV_ABS 17 ABS_HAT0Y +1` |
| D-pad left | `EV_ABS 16 ABS_HAT0X -1` |
| D-pad right | `EV_ABS 16 ABS_HAT0X +1` |
| Menu/M | SDL raw joystick `button 8` and `button 11` on one press |

The separate Menu/M control produces no event on `/dev/input/event1` or
`/dev/input/event2`. The frontend handles raw joystick `button 8` as Menu and
ignores the duplicate `button 11` from the same physical press. Do not assign
the unverified `BTN_TL2` or `KEY_GOTO` capabilities as evdev controls.
