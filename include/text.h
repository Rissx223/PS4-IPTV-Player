// ============================================================================
//  text.h - FreeType-backed text rendering onto an SDL renderer.
// ============================================================================
#ifndef PS4_IPTV_TEXT_H
#define PS4_IPTV_TEXT_H

#include <SDL2/SDL.h>
// The OpenOrbis toolchain exposes a usable FreeType API through this umbrella
// header (the raw <freetype/freetype.h> in the sysroot is only partially
// usable); it also pulls in the real struct/constant definitions.
#include <proto-include.h>
#include <string>

struct Color { uint8_t r, g, b, a; };

class Font {
public:
    bool  load(const char *path, int pixelSize);
    void  unload();

    // Render a UTF-8/ASCII string; returns the advance width in pixels.
    int   draw(SDL_Renderer *r, int x, int y, const std::string &text, Color color);
    // Measure without drawing.
    int   measure(const std::string &text) const;
    // Draw clipped/ellipsized to fit maxWidth pixels.
    int   drawClipped(SDL_Renderer *r, int x, int y, const std::string &text,
                      Color color, int maxWidth);

    int   lineHeight() const { return m_lineHeight; }
    bool  valid() const { return m_face != nullptr; }

private:
    FT_Face m_face = nullptr;
    int     m_lineHeight = 0;
    int     m_ascender = 0;
};

// Load the FreeType system module + library. Call once at startup.
bool text_global_init();

#endif // PS4_IPTV_TEXT_H
