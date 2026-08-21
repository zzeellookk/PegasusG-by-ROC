# Third-Party Notices

This file records the known provenance of bundled or referenced components.
The source repository for each component is the authoritative place for its
complete license and source code.

## Pegasus Frontend

- Project: https://github.com/mmatyas/pegasus-frontend
- Revision used as the reference baseline: `6b322063a036db60cba5810fda82a3ce38f1e62f`
- License: GPLv3 with supplementary trademark terms.
- Full text: `LICENSE.md`.

## Bundled Libretro Cores

| Binary | Upstream source | Revision | License |
| --- | --- | --- | --- |
| `assets/cores/mgba_libretro.so` | https://github.com/libretro/mgba | `e31759b24e7a4e3899285ff720d7b573ac328ae7` | MPL-2.0 |
| `assets/cores/gpsp_libretro.so` | https://github.com/libretro/gpsp | `5b6e751f4abf368509146cd143c949c1946ac1ae` | GPL-2.0 |
| `assets/cores/vbam_libretro.so` | https://github.com/libretro/vbam-libretro | `e8b2875d6cad10fc3c7c9f57bb5f1acc324d7c10` | GPL-2.0 and component notices |
| `assets/cores/vba_next_libretro.so` | https://github.com/libretro/vba-next | `2b96fd3a77025f3083daf61126b1852d5e0eace7` | GPL-2.0 |
| `assets/cores/gpsp_rumble_libretro.so` | User-provided gpSP rumble variant | Verify with the corresponding source package before redistribution | Must follow the variant's source license |
| `Brick/cores/gpsp_rumble_libretro.so` | https://github.com/libretro/gpsp | `5b6e751f4abf368509146cd143c949c1946ac1ae` | GPL-2.0; built by Brick CI |

License texts available locally:

- `third_party/licenses/mGBA-MPL-2.0.txt`
- `third_party/licenses/gpSP-GPL-2.0.txt`
- `third_party/licenses/VBA-M-License.txt`
- `third_party/licenses/VBA-Next-GPL-2.0.txt`

The exact binary hashes currently stored in this repository are recorded here
to make replacement and source matching auditable:

```text
gpsp_libretro.so         SHA256 289FB160621A545A9C6FE206B9723A9003175921EDDCD43F4D67F6A178F18976
gpsp_rumble_libretro.so  SHA256 84B2122E2D62C665583ABE169694721E714D735549167BA2F42D8C37F4C08D09
mgba_libretro.so         SHA256 0424BB9C5D6A1654E8D4AE8D1E013DD3F6B414810A480D71E60F3B5E99772AD6
vbam_libretro.so         SHA256 71DEAE006763A9A5128D66C51DE5DF60D4F945D1B1DD4BCF2330E174B80DF122
vba_next_libretro.so     SHA256 FA56172A8BF69762936A8328432519B1310BC0F49C5BA69239376F4C94EBE566
```

## Other Assets

- SDL2, SDL2_image, SDL2_ttf, ALSA and FFmpeg are supplied by the build host
  or target firmware and retain their own licenses.
- The WLF music tracks in `assets/music/builtin/` are retained as supplied by
  the original configuration package. Redistribution rights have not been
  independently verified; obtain permission or replace them before a public
  commercial distribution.
- `assets/ui/`, splash images, filter presets and shaders contain project or
  user-provided artwork. Keep their attribution and verify permission for any
  public redistribution.
- `assets/cheats/gba-auto-cheats.zip` is a data collection, not source code;
  its individual entries may have separate authors and redistribution terms.
