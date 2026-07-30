#include "text.h"

#include <orbis/Sysmodule.h>
#include <vector>

static FT_Library g_ft = nullptr;

bool text_global_init()
{
    if (g_ft) return true;
    // The OpenOrbis SDL build links FreeType but the module must be loaded.
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL);
    if (FT_Init_FreeType(&g_ft) != 0) {
        g_ft = nullptr;
        return false;
    }
    return true;
}

bool Font::load(const char *path, int pixelSize)
{
    if (!g_ft) return false;
    if (FT_New_Face(g_ft, path, 0, &m_face) != 0) {
        m_face = nullptr;
        return false;
    }
    FT_Set_Pixel_Sizes(m_face, 0, pixelSize);
    m_lineHeight = (int)(m_face->size->metrics.height >> 6);
    m_ascender   = (int)(m_face->size->metrics.ascender >> 6);
    if (m_lineHeight <= 0) m_lineHeight = pixelSize + pixelSize / 4;
    if (m_ascender   <= 0) m_ascender   = pixelSize;
    return true;
}

void Font::unload()
{
    if (m_face) {
        FT_Done_Face(m_face);
        m_face = nullptr;
    }
}

int Font::measure(const std::string &text) const
{
    if (!m_face) return 0;
    int width = 0;
    for (unsigned char ch : text) {
        FT_UInt gi = FT_Get_Char_Index(m_face, ch);
        if (FT_Load_Glyph(m_face, gi, FT_LOAD_DEFAULT) != 0)
            continue;
        width += (int)(m_face->glyph->advance.x >> 6);
    }
    return width;
}

// Rasterise the string into an ARGB surface with per-pixel alpha coverage.
static SDL_Surface *rasterize(FT_Face face, int ascender, const std::string &text,
                              Color color, int &outAdvance)
{
    if (!face) { outAdvance = 0; return nullptr; }

    // First pass: compute the pixel extents.
    int penX = 0;
    int minTop = 0, maxBottom = 0;
    for (unsigned char ch : text) {
        FT_UInt gi = FT_Get_Char_Index(face, ch);
        if (FT_Load_Glyph(face, gi, FT_LOAD_DEFAULT) != 0) continue;
        FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        int top    = ascender - face->glyph->bitmap_top;
        int bottom = top + (int)face->glyph->bitmap.rows;
        if (top < minTop) minTop = top;
        if (bottom > maxBottom) maxBottom = bottom;
        penX += (int)(face->glyph->advance.x >> 6);
    }
    outAdvance = penX;

    int w = penX;
    int h = (maxBottom > minTop) ? (maxBottom - minTop) : 1;
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;

    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surf) return nullptr;
    SDL_memset(surf->pixels, 0, (size_t)surf->pitch * surf->h);

    uint32_t *pixels = (uint32_t *)surf->pixels;
    int stride = surf->pitch / 4;

    // Second pass: composite glyph coverage as alpha.
    penX = 0;
    for (unsigned char ch : text) {
        FT_UInt gi = FT_Get_Char_Index(face, ch);
        if (FT_Load_Glyph(face, gi, FT_LOAD_DEFAULT) != 0) continue;
        FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        FT_Bitmap &bm = face->glyph->bitmap;

        int glyphX = penX + face->glyph->bitmap_left;
        int glyphY = (ascender - face->glyph->bitmap_top) - minTop;

        for (int row = 0; row < (int)bm.rows; row++) {
            int py = glyphY + row;
            if (py < 0 || py >= h) continue;
            for (int col = 0; col < (int)bm.width; col++) {
                int px = glyphX + col;
                if (px < 0 || px >= w) continue;
                uint8_t cov = bm.buffer[row * bm.pitch + col];
                if (!cov) continue;
                uint32_t a = ((uint32_t)cov * color.a) / 255;
                pixels[py * stride + px] =
                    (a << 24) | ((uint32_t)color.r << 16) |
                    ((uint32_t)color.g << 8) | (uint32_t)color.b;
            }
        }
        penX += (int)(face->glyph->advance.x >> 6);
    }
    return surf;
}

int Font::draw(SDL_Renderer *r, int x, int y, const std::string &text, Color color)
{
    if (!m_face || text.empty()) return 0;
    int advance = 0;
    SDL_Surface *surf = rasterize(m_face, m_ascender, text, color, advance);
    if (!surf) return 0;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_Rect dst = { x, y, surf->w, surf->h };
        SDL_RenderCopy(r, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
    return advance;
}

int Font::drawClipped(SDL_Renderer *r, int x, int y, const std::string &text,
                      Color color, int maxWidth)
{
    if (!m_face) return 0;
    if (measure(text) <= maxWidth)
        return draw(r, x, y, text, color);

    // Trim characters until "text..." fits.
    std::string ell = "...";
    std::string cut = text;
    while (!cut.empty() && measure(cut + ell) > maxWidth)
        cut.pop_back();
    return draw(r, x, y, cut + ell, color);
}
