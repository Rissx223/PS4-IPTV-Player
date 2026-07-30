// ============================================================================
//  keyboard.h - Self-contained on-screen keyboard navigable with the D-pad.
//
//  Avoids a dependency on the system IME dialog so the UI is fully rendered by
//  the app. Used for entering panel hosts, credentials and playlist URLs.
// ============================================================================
#ifndef PS4_IPTV_KEYBOARD_H
#define PS4_IPTV_KEYBOARD_H

#include <SDL2/SDL.h>
#include <string>
#include "input.h"
#include "text.h"

class OnScreenKeyboard {
public:
    void begin(const std::string &title, const std::string &initial, bool password);

    // Feed a logical action. Returns true while the keyboard remains open.
    // When editing finishes, `done` is set and `accepted` indicates OK vs cancel.
    bool handle(Action a, bool &done, bool &accepted);

    void render(SDL_Renderer *r, Font &font, Font &bigFont, int screenW, int screenH);

    const std::string &text() const { return m_text; }

private:
    void commitCurrentKey();

    std::string m_title;
    std::string m_text;
    bool        m_password = false;
    bool        m_shift    = false;
    int         m_row = 0;
    int         m_col = 0;
};

#endif // PS4_IPTV_KEYBOARD_H
