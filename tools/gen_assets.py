#!/usr/bin/env python3
"""Generate the app icon and UI images for PS4 IPTV Player.

Run from the repo root:  python3 tools/gen_assets.py
Requires Pillow.
"""
import math
import os
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG_DIR = os.path.join(ROOT, "assets", "images")
SCE_DIR = os.path.join(ROOT, "sce_sys")
os.makedirs(IMG_DIR, exist_ok=True)
os.makedirs(SCE_DIR, exist_ok=True)

ACCENT   = (74, 134, 240)
ACCENT2  = (120, 92, 240)
DARK     = (14, 16, 24)
DARK2    = (26, 30, 44)
WHITE    = (236, 239, 246)


def load_font(size):
    for path in [
        os.path.join(ROOT, "assets", "fonts", "Gontserrat-Regular.ttf"),
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    ]:
        if os.path.exists(path):
            try:
                return ImageFont.truetype(path, size)
            except Exception:
                pass
    return ImageFont.load_default()


def vgrad(w, h, top, bottom):
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        t = y / max(1, h - 1)
        px_row = tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3))
        for x in range(w):
            px[x, y] = px_row
    return img


def diag_grad(size, c1, c2):
    w = h = size
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            t = (x + y) / (2 * (size - 1))
            px[x, y] = tuple(int(c1[i] + (c2[i] - c1[i]) * t) for i in range(3))
    return img


def rounded_mask(size, radius):
    m = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(m)
    d.rounded_rectangle([0, 0, size - 1, size - 1], radius=radius, fill=255)
    return m


def draw_play_triangle(d, cx, cy, r, color):
    pts = [(cx - r * 0.5, cy - r), (cx - r * 0.5, cy + r), (cx + r, cy)]
    d.polygon(pts, fill=color)


def draw_signal_arcs(d, cx, cy, base_r, color, width):
    for i in range(1, 4):
        rr = base_r * i
        d.arc([cx - rr, cy - rr, cx + rr, cy + rr], start=-135, end=-45,
              fill=color, width=width)


def make_icon(size, out, radius_frac=0.18):
    bg = diag_grad(size, ACCENT2, ACCENT)
    # subtle vignette
    icon = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    icon.paste(bg, (0, 0))
    d = ImageDraw.Draw(icon)

    # TV screen panel
    pad = int(size * 0.20)
    screen = [pad, int(size * 0.26), size - pad, int(size * 0.70)]
    d.rounded_rectangle(screen, radius=int(size * 0.04), fill=DARK2,
                        outline=WHITE, width=max(2, size // 120))
    # play button in screen
    cx = size / 2
    cy = (screen[1] + screen[3]) / 2
    draw_play_triangle(d, cx, cy, size * 0.10, WHITE)
    # signal arcs above
    draw_signal_arcs(d, cx, screen[1] - int(size * 0.02), size * 0.06,
                     WHITE, max(2, size // 90))
    # stand
    d.rectangle([cx - size * 0.02, screen[3], cx + size * 0.02, screen[3] + size * 0.06], fill=DARK2)
    d.rounded_rectangle([cx - size * 0.12, screen[3] + size * 0.06,
                         cx + size * 0.12, screen[3] + size * 0.09],
                        radius=int(size * 0.015), fill=DARK2)

    # round the whole icon
    mask = rounded_mask(size, int(size * radius_frac))
    out_img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    out_img.paste(icon, (0, 0), mask)
    out_img.save(out)
    print("wrote", out)


def make_logo():
    size = 128
    make_icon(size, os.path.join(IMG_DIR, "logo.png"), radius_frac=0.22)


def make_background():
    w, h = 1920, 1080
    img = vgrad(w, h, (12, 14, 22), (22, 26, 42)).convert("RGBA")
    d = ImageDraw.Draw(img, "RGBA")
    # soft accent glow top-left
    glow = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    gd.ellipse([-400, -400, 700, 700], fill=(74, 134, 240, 60))
    gd.ellipse([w - 700, h - 700, w + 400, h + 400], fill=(120, 92, 240, 45))
    glow = glow.filter(ImageFilter.GaussianBlur(160))
    img = Image.alpha_composite(img, glow)
    img.convert("RGB").save(os.path.join(IMG_DIR, "background.png"))
    print("wrote background.png")


def make_placeholder():
    w, h = 192, 108
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([0, 0, w - 1, h - 1], radius=8, fill=DARK2,
                        outline=(60, 66, 84), width=2)
    draw_play_triangle(d, w / 2, h / 2, 18, (120, 130, 150))
    img.save(os.path.join(IMG_DIR, "placeholder.png"))
    print("wrote placeholder.png")


def make_type_icon(name, glyph_fn):
    size = 112
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([0, 0, size - 1, size - 1], radius=22,
                        fill=DARK2, outline=ACCENT, width=3)
    glyph_fn(d, size)
    img.save(os.path.join(IMG_DIR, name))
    print("wrote", name)


def glyph_xtream(d, s):
    f = load_font(int(s * 0.30))
    txt = "API"
    try:
        bb = d.textbbox((0, 0), txt, font=f)
        tw, th = bb[2] - bb[0], bb[3] - bb[1]
    except Exception:
        tw, th = f.getsize(txt)
    d.text(((s - tw) / 2, (s - th) / 2 - s * 0.14), txt, font=f, fill=WHITE)
    draw_signal_arcs(d, s / 2, s * 0.72, s * 0.10, ACCENT, 5)


def glyph_m3u(d, s):
    f = load_font(int(s * 0.26))
    txt = "M3U"
    try:
        bb = d.textbbox((0, 0), txt, font=f)
        tw, th = bb[2] - bb[0], bb[3] - bb[1]
    except Exception:
        tw, th = f.getsize(txt)
    d.text(((s - tw) / 2, (s - th) / 2), txt, font=f, fill=WHITE)


def glyph_local(d, s):
    # folder glyph
    d.rounded_rectangle([s * 0.22, s * 0.40, s * 0.78, s * 0.72], radius=6, fill=ACCENT)
    d.rectangle([s * 0.22, s * 0.34, s * 0.48, s * 0.44], fill=ACCENT)


def main():
    make_icon(512, os.path.join(SCE_DIR, "icon0.png"))
    make_logo()
    make_background()
    make_placeholder()
    make_type_icon("icon_xtream.png", glyph_xtream)
    make_type_icon("icon_m3u.png", glyph_m3u)
    make_type_icon("icon_local.png", glyph_local)


if __name__ == "__main__":
    main()
