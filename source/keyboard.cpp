#include "keyboard.h"

namespace {

// Key layout. Special tokens are handled in commitCurrentKey().
const char *kRowsLower[] = {
    "1234567890",
    "qwertyuiop",
    "asdfghjkl:",
    "zxcvbnm./-",
    "@_~ "          // last row: @ _ ~ space + specials appended in code
};
const char *kRowsUpper[] = {
    "!@#$%^&*()",
    "QWERTYUIOP",
    "ASDFGHJKL;",
    "ZXCVBNM,?+",
    "=?:/ "
};

const int kNumRows = 5;

// Virtual columns beyond the character grid: Shift, Backspace, Cancel, OK.
enum { COL_CHARS = 10 };

} // namespace

void OnScreenKeyboard::begin(const std::string &title, const std::string &initial, bool password)
{
    m_title    = title;
    m_text     = initial;
    m_password = password;
    m_shift    = false;
    m_row = 0;
    m_col = 0;
}

void OnScreenKeyboard::commitCurrentKey()
{
    const char **rows = m_shift ? kRowsUpper : kRowsLower;

    if (m_col < COL_CHARS) {
        const char *row = rows[m_row];
        int len = (int)SDL_strlen(row);
        if (m_col < len) {
            char c = row[m_col];
            if (c != ' ' || true) // allow spaces too
                m_text.push_back(c);
        }
    }
}

bool OnScreenKeyboard::handle(Action a, bool &done, bool &accepted)
{
    done = false;
    accepted = false;

    // Columns 0..9 are characters; 10..13 are Shift/Back/Cancel/OK.
    const int totalCols = COL_CHARS + 4;

    switch (a) {
        case Action::Up:    m_row = (m_row + kNumRows - 1) % kNumRows; break;
        case Action::Down:  m_row = (m_row + 1) % kNumRows; break;
        case Action::Left:  m_col = (m_col + totalCols - 1) % totalCols; break;
        case Action::Right: m_col = (m_col + 1) % totalCols; break;

        case Action::Confirm: {
            if (m_col < COL_CHARS) {
                commitCurrentKey();
            } else {
                switch (m_col - COL_CHARS) {
                    case 0: m_shift = !m_shift; break;                 // Shift
                    case 1: if (!m_text.empty()) m_text.pop_back(); break; // Backspace
                    case 2: done = true; accepted = false; return false;   // Cancel
                    case 3: done = true; accepted = true;  return false;   // OK
                }
            }
            break;
        }
        case Action::Action1: // Square = quick backspace
            if (!m_text.empty()) m_text.pop_back();
            break;
        case Action::Action2: // Triangle = toggle shift
            m_shift = !m_shift;
            break;
        case Action::Menu:    // Options = accept
            done = true; accepted = true; return false;
        case Action::Back:    // Circle = cancel
            done = true; accepted = false; return false;
        default: break;
    }
    return true;
}

void OnScreenKeyboard::render(SDL_Renderer *r, Font &font, Font &bigFont,
                              int screenW, int screenH)
{
    // Dim background.
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 200);
    SDL_Rect full = { 0, 0, screenW, screenH };
    SDL_RenderFillRect(r, &full);

    const int panelW = 900;
    const int panelH = 520;
    const int px = (screenW - panelW) / 2;
    const int py = (screenH - panelH) / 2;

    SDL_SetRenderDrawColor(r, 24, 28, 40, 255);
    SDL_Rect panel = { px, py, panelW, panelH };
    SDL_RenderFillRect(r, &panel);
    SDL_SetRenderDrawColor(r, 80, 120, 220, 255);
    SDL_RenderDrawRect(r, &panel);

    Color white = { 235, 238, 245, 255 };
    Color dim   = { 150, 158, 175, 255 };
    Color hi    = { 20, 24, 34, 255 };

    bigFont.draw(r, px + 30, py + 20, m_title, white);

    // Text field
    SDL_SetRenderDrawColor(r, 12, 14, 20, 255);
    SDL_Rect field = { px + 30, py + 80, panelW - 60, 56 };
    SDL_RenderFillRect(r, &field);
    SDL_SetRenderDrawColor(r, 60, 66, 84, 255);
    SDL_RenderDrawRect(r, &field);

    std::string shown = m_password ? std::string(m_text.size(), '*') : m_text;
    font.draw(r, field.x + 14, field.y + 14, shown + "_", white);

    // Key grid
    const char **rows = m_shift ? kRowsUpper : kRowsLower;
    const int keyW = 66, keyH = 60, gapX = 8, gapY = 8;
    const int gridX = px + 30;
    const int gridY = py + 160;

    for (int row = 0; row < kNumRows; row++) {
        for (int col = 0; col < COL_CHARS; col++) {
            const char *rs = rows[row];
            if (col >= (int)SDL_strlen(rs)) continue;
            char c = rs[col];

            SDL_Rect key = { gridX + col * (keyW + gapX),
                             gridY + row * (keyH + gapY), keyW, keyH };
            bool sel = (row == m_row && col == m_col);
            if (sel) SDL_SetRenderDrawColor(r, 80, 140, 240, 255);
            else     SDL_SetRenderDrawColor(r, 40, 46, 62, 255);
            SDL_RenderFillRect(r, &key);

            char label[2] = { c, 0 };
            font.draw(r, key.x + 24, key.y + 16, label, sel ? hi : white);
        }
    }

    // Function keys column (Shift / Back / Cancel / OK)
    const char *fnLabels[4] = { "Shift", "Back", "Cancel", "OK" };
    const int fnX = gridX + COL_CHARS * (keyW + gapX) + 10;
    for (int i = 0; i < 4; i++) {
        SDL_Rect key = { fnX, gridY + i * (keyH + gapY), 150, keyH };
        bool sel = (m_col == COL_CHARS + i);
        if (sel)      SDL_SetRenderDrawColor(r, 80, 140, 240, 255);
        else if (i==3)SDL_SetRenderDrawColor(r, 40, 120, 70, 255);
        else          SDL_SetRenderDrawColor(r, 40, 46, 62, 255);
        SDL_RenderFillRect(r, &key);
        font.draw(r, key.x + 16, key.y + 16, fnLabels[i], sel ? hi : white);
    }

    font.draw(r, px + 30, py + panelH - 40,
              "D-pad: move   X: select   /\\: shift   []: backspace   O: cancel   Options: OK",
              dim);
}
