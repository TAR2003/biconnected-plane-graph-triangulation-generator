#!/usr/bin/env python3
"""
make_image_pdfs.py

Scans a parent directory containing subfolders of graph images and creates
one PDF per subfolder, laying out multiple images per page in a grid.

Usage:
    python make_image_pdfs.py /path/to/output-images
    python make_image_pdfs.py /path/to/output-images --cols 4 --rows 5
    python make_image_pdfs.py /path/to/output-images --out /path/to/pdfs --cols 3 --rows 4

Dependencies:
    pip install pillow reportlab --break-system-packages
"""

import os
import sys
import io
import argparse
from pathlib import Path

from PIL import Image
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.pdfgen import canvas
from reportlab.lib.utils import ImageReader

IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tif", ".tiff", ".webp"}


def find_images(folder: Path):
    """Return sorted list of image file paths in a folder (natural-ish sort)."""
    files = [p for p in folder.iterdir() if p.suffix.lower() in IMAGE_EXTS]

    def sort_key(p: Path):
        # Split into digit / non-digit chunks for a natural sort
        import re
        parts = re.split(r"(\d+)", p.stem)
        return [int(part) if part.isdigit() else part.lower() for part in parts]

    return sorted(files, key=sort_key)


def load_downsampled(img_path: Path, target_w_px: int, target_h_px: int, jpeg_quality: int):
    """
    Open image, downsample it to roughly match its on-page display size
    (at target_w_px x target_h_px, ~ the DPI we want), and return an
    in-memory JPEG buffer wrapped as an ImageReader. This is what keeps
    file size small -- we never embed the original full-res image.
    """
    with Image.open(img_path) as im:
        im = im.convert("RGB")
        iw, ih = im.size

        # Don't upscale small images, only downscale large ones
        scale = min(target_w_px / iw, target_h_px / ih, 1.0)
        new_w = max(1, int(iw * scale))
        new_h = max(1, int(ih * scale))
        if scale < 1.0:
            im = im.resize((new_w, new_h), Image.LANCZOS)

        buf = io.BytesIO()
        im.save(buf, format="JPEG", quality=jpeg_quality, optimize=True)
        buf.seek(0)
        return ImageReader(buf), im.size


def make_pdf_for_folder(folder: Path, out_pdf: Path, cols: int, rows: int,
                         page_size=A4, margin_mm: float = 8, gap_mm: float = 4,
                         title: str = None, target_dpi: int = 110, jpeg_quality: int = 60):
    images = find_images(folder)
    if not images:
        print(f"  [skip] no images found in {folder}")
        return 0

    page_w, page_h = page_size
    margin = margin_mm * mm
    gap = gap_mm * mm

    title_h = 14 * mm if title else 0

    grid_w = page_w - 2 * margin
    grid_h = page_h - 2 * margin - title_h

    cell_w = (grid_w - (cols - 1) * gap) / cols
    cell_h = (grid_h - (rows - 1) * gap) / rows

    per_page = cols * rows

    c = canvas.Canvas(str(out_pdf), pagesize=page_size)

    total_pages = (len(images) + per_page - 1) // per_page

    for page_idx in range(total_pages):
        page_images = images[page_idx * per_page:(page_idx + 1) * per_page]

        if title:
            c.setFont("Helvetica-Bold", 12)
            page_title = f"{title}  (page {page_idx + 1}/{total_pages})"
            c.drawString(margin, page_h - margin - 9, page_title)

        for i, img_path in enumerate(page_images):
            col = i % cols
            row = i // cols

            x = margin + col * (cell_w + gap)
            y = page_h - margin - title_h - (row + 1) * cell_h - row * gap

            # Compute target pixel resolution based on cell size (in points)
            # and desired dpi, so we never embed more resolution than will
            # actually be visible on the page.
            target_w_px = int(cell_w / 72 * target_dpi)
            target_h_px = int(cell_h / 72 * target_dpi)

            try:
                img_reader, (rw, rh) = load_downsampled(
                    img_path, target_w_px, target_h_px, jpeg_quality
                )
            except Exception as e:
                print(f"    [warn] could not read {img_path.name}: {e}")
                continue

            # Fit image inside cell, preserving aspect ratio
            scale = min(cell_w / rw, cell_h / rh)
            draw_w = rw * scale
            draw_h = rh * scale
            offset_x = x + (cell_w - draw_w) / 2
            offset_y = y + (cell_h - draw_h) / 2

            c.drawImage(img_reader, offset_x, offset_y, width=draw_w, height=draw_h,
                        preserveAspectRatio=True, anchor='c')

            # Caption (filename) under image, small font
            c.setFont("Helvetica", 5.5)
            caption = img_path.stem
            if len(caption) > 40:
                caption = caption[:37] + "..."
            c.drawCentredString(x + cell_w / 2, y - 2, caption)

        c.showPage()

    c.save()
    print(f"  [ok] {out_pdf.name}  ({len(images)} images, {total_pages} pages)")
    return len(images)


def main():
    parser = argparse.ArgumentParser(description="Create one grid-layout PDF per image subfolder.")
    parser.add_argument("input_dir", type=str, help="Parent directory containing image subfolders")
    parser.add_argument("--out", type=str, default=None,
                         help="Output directory for PDFs (default: <input_dir>/pdfs)")
    parser.add_argument("--cols", type=int, default=4, help="Images per row (default: 4)")
    parser.add_argument("--rows", type=int, default=5, help="Rows per page (default: 5)")
    parser.add_argument("--no-title", action="store_true", help="Don't print a title/header on each page")
    parser.add_argument("--dpi", type=int, default=110,
                         help="Target resolution for embedded images (default: 110). "
                              "Lower = smaller file / lower quality (try 72-96 for very small files).")
    parser.add_argument("--quality", type=int, default=60,
                         help="JPEG quality 1-95 (default: 60). Lower = smaller file / more compression artifacts.")
    args = parser.parse_args()

    input_dir = Path(args.input_dir).expanduser().resolve()
    if not input_dir.is_dir():
        print(f"Error: {input_dir} is not a directory")
        sys.exit(1)

    out_dir = Path(args.out).expanduser().resolve() if args.out else (input_dir / "pdfs")
    out_dir.mkdir(parents=True, exist_ok=True)

    subfolders = sorted([p for p in input_dir.iterdir() if p.is_dir() and p.name != "pdfs"])

    if not subfolders:
        print(f"No subfolders found in {input_dir}")
        sys.exit(1)

    print(f"Found {len(subfolders)} folders. Grid: {args.cols} cols x {args.rows} rows "
          f"({args.cols * args.rows} images/page)")
    print(f"Output PDFs -> {out_dir}\n")

    total_images = 0
    for folder in subfolders:
        print(f"Processing: {folder.name}")
        out_pdf = out_dir / f"{folder.name}.pdf"
        title = None if args.no_title else folder.name
        total_images += make_pdf_for_folder(
            folder, out_pdf, cols=args.cols, rows=args.rows, title=title,
            target_dpi=args.dpi, jpeg_quality=args.quality
        )

    print(f"\nDone. {total_images} images total across {len(subfolders)} PDFs.")


if __name__ == "__main__":
    main()