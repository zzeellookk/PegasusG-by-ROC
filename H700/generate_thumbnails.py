#!/usr/bin/env python3
"""Generate compact cover and logo textures for the H700 frontend."""

from __future__ import annotations

import argparse
import os
from concurrent.futures import ThreadPoolExecutor
from functools import partial
from pathlib import Path

from PIL import Image, ImageOps


SOURCE_NAMES = {
    "boxfront.png", "cover.png", "boxfront.jpg", "cover.jpg",
    "logo.png", "logo.jpg",
}
DEFAULT_COVER_SIZE = 192
DEFAULT_LOGO_WIDTH = 256
DEFAULT_LOGO_HEIGHT = 96


def generate(source: Path, force: bool, cover_size: int,
             logo_width: int, logo_height: int) -> bool:
    output = source.with_name(f"{source.stem}.h700.bmp")
    if not force and output.exists() and output.stat().st_mtime >= source.stat().st_mtime:
        return False

    is_logo = source.name.lower().startswith("logo.")
    target = (logo_width, logo_height) if is_logo else (cover_size, cover_size)

    with Image.open(source) as image:
        image = ImageOps.exif_transpose(image).convert("RGB")
        image.thumbnail(target, Image.Resampling.LANCZOS)
        canvas = Image.new("RGB", target, "black")
        canvas.paste(image, ((target[0] - image.width) // 2,
                             (target[1] - image.height) // 2))
        temporary = output.with_suffix(output.suffix + ".tmp")
        canvas.save(temporary, format="BMP")
        temporary.replace(output)
    return True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path, help="Pegasus content root containing media")
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--size", type=int, default=DEFAULT_COVER_SIZE,
                        help="square cover texture size")
    parser.add_argument("--logo-width", type=int, default=DEFAULT_LOGO_WIDTH)
    parser.add_argument("--logo-height", type=int, default=DEFAULT_LOGO_HEIGHT)
    parser.add_argument("--workers", type=int, default=min(8, os.cpu_count() or 1))
    args = parser.parse_args()
    if args.size < 64 or args.size > 512:
        parser.error("--size must be between 64 and 512")
    if args.logo_width < 64 or args.logo_width > 512:
        parser.error("--logo-width must be between 64 and 512")
    if args.logo_height < 32 or args.logo_height > 256:
        parser.error("--logo-height must be between 32 and 256")
    if args.workers < 1:
        parser.error("--workers must be at least 1")

    sources = [source for source in args.root.rglob("*")
               if source.is_file() and source.name.lower() in SOURCE_NAMES]
    worker = partial(generate, force=args.force, cover_size=args.size,
                     logo_width=args.logo_width, logo_height=args.logo_height)
    with ThreadPoolExecutor(max_workers=args.workers) as executor:
        results = list(executor.map(worker, sources))
    created = sum(results)
    cover_count = sum(not source.name.lower().startswith("logo.") for source in sources)
    logo_count = len(sources) - cover_count
    print(f"cover_size={args.size} logo_size={args.logo_width}x{args.logo_height} "
          f"covers={cover_count} logos={logo_count} sources={len(sources)} created={created} "
          f"skipped={len(sources) - created}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
