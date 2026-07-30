#include "app.h"

namespace {
const Color kWhite  = { 235, 238, 245, 255 };
const Color kDim    = { 150, 158, 175, 255 };
const Color kAccent = { 80, 140, 240, 255 };
const Color kGreen  = { 90, 200, 120, 255 };
const Color kRed    = { 235, 90, 90, 255 };
const Color kPanel  = { 28, 32, 46, 255 };
const Color kPanel2 = { 40, 46, 62, 255 };
const Color kDark   = { 14, 16, 24, 255 };
const Color kSel    = { 80, 140, 240, 255 };

const char *codecBadge(StreamCodec c) { return codec_name(c); }
} // namespace

void App::render()
{
    // Background
    SDL_SetRenderDrawColor(m_r, 10, 12, 18, 255);
    SDL_RenderClear(m_r);
    if (m_texBackground) {
        SDL_Rect full = { 0, 0, m_w, m_h };
        SDL_RenderCopy(m_r, m_texBackground, nullptr, &full);
    }

    switch (m_screen) {
        case Screen::Home:       renderHome(); break;
        case Screen::AddType:    renderAddType(); break;
        case Screen::Form:       renderForm(); break;
        case Screen::Browse:     renderBrowse(); break;
        case Screen::PlayerView: renderPlayer(); break;
        case Screen::Message:
            // Draw the underlying screen dimmed, then the message box.
            renderHome();
            renderMessage();
            break;
    }

    if (m_keyboardActive)
        m_keyboard.render(m_r, m_font, m_fontBig, m_w, m_h);
}

void App::renderTopBar(const std::string &subtitle)
{
    fillRect(0, 0, m_w, 96, kDark);
    if (m_texLogo) {
        SDL_Rect logo = { 40, 20, 56, 56 };
        SDL_RenderCopy(m_r, m_texLogo, nullptr, &logo);
    }
    m_fontBig.draw(m_r, 112, 24, "PS4 IPTV Player", kWhite);
    if (!subtitle.empty())
        m_font.draw(m_r, m_w - m_font.measure(subtitle) - 40, 34, subtitle, kDim);
    fillRect(0, 96, m_w, 2, kAccent);
}

void App::renderHome()
{
    renderTopBar("Home");

    int x = 120, y = 150;
    const int rowH = 84, rowW = m_w - 240;

    m_font.draw(m_r, x, y, "Sources", kDim);
    y += 44;

    int total = (int)m_sources.size() + 3;
    for (int i = 0; i < total; i++) {
        bool sel = (i == m_homeSel);
        Color bg = sel ? kSel : kPanel;
        fillRect(x, y, rowW, rowH - 12, bg);

        SDL_Texture *icon = nullptr;
        std::string title, sub;

        if (i < (int)m_sources.size()) {
            const SourceProfile &s = m_sources[i];
            title = s.name.empty() ? "(unnamed)" : s.name;
            switch (s.kind) {
                case SourceKind::Xtream:    icon = m_texXtream; sub = "Xtream API  " + s.host; break;
                case SourceKind::M3uUrl:    icon = m_texM3u;    sub = "M3U URL  " + s.url; break;
                case SourceKind::LocalFile: icon = m_texLocal;  sub = "Local File  " + s.url; break;
            }
        } else {
            int add = i - (int)m_sources.size();
            if (add == 0) { title = "+ Add Xtream API source"; icon = m_texXtream; }
            else if (add == 1) { title = "+ Add M3U URL"; icon = m_texM3u; }
            else { title = "+ Add local playlist"; icon = m_texLocal; }
        }

        if (icon) {
            SDL_Rect ir = { x + 16, y + 8, 56, 56 };
            SDL_RenderCopy(m_r, icon, nullptr, &ir);
        }
        Color tc = sel ? kDark : kWhite;
        m_font.draw(m_r, x + 90, y + 10, title, tc);
        if (!sub.empty())
            m_fontSmall.drawClipped(m_r, x + 90, y + 42, sub, sel ? kDark : kDim, rowW - 120);

        y += rowH;
    }

    m_fontSmall.draw(m_r, 120, m_h - 50,
        "X: open   []: edit   /\\: delete   O: exit", kDim);
}

void App::renderAddType()
{
    renderTopBar("Add Source");
    const char *labels[3] = { "Xtream API (panel login)", "M3U URL", "Local Playlist file" };
    SDL_Texture *icons[3] = { m_texXtream, m_texM3u, m_texLocal };

    int x = 200, y = 220;
    for (int i = 0; i < 3; i++) {
        bool sel = (i == m_addTypeSel);
        fillRect(x, y, m_w - 400, 100, sel ? kSel : kPanel);
        if (icons[i]) {
            SDL_Rect ir = { x + 20, y + 20, 60, 60 };
            SDL_RenderCopy(m_r, icons[i], nullptr, &ir);
        }
        m_font.draw(m_r, x + 100, y + 32, labels[i], sel ? kDark : kWhite);
        y += 130;
    }
    m_fontSmall.draw(m_r, x, m_h - 60, "X: choose   O: back", kDim);
}

void App::renderForm()
{
    renderTopBar("Add / Edit Source");

    // Rebuild the same field list the input handler uses.
    struct F { std::string label; std::string value; bool pass; bool save; };
    std::vector<F> fields;
    fields.push_back({ "Name", m_draft.name, false, false });
    if (m_draft.kind == SourceKind::Xtream) {
        fields.push_back({ "Host (http://panel:port)", m_draft.host, false, false });
        fields.push_back({ "Username", m_draft.username, false, false });
        fields.push_back({ "Password", m_draft.password, true, false });
    } else if (m_draft.kind == SourceKind::M3uUrl) {
        fields.push_back({ "Playlist URL", m_draft.url, false, false });
    } else {
        fields.push_back({ "File path (/data/...)", m_draft.url, false, false });
    }
    fields.push_back({ "Save", "", false, true });

    const char *kn = (m_draft.kind == SourceKind::Xtream) ? "Xtream API" :
                     (m_draft.kind == SourceKind::M3uUrl) ? "M3U URL" : "Local Playlist";
    m_font.draw(m_r, 200, 150, std::string("Type: ") + kn, kDim);

    int x = 200, y = 210;
    for (int i = 0; i < (int)fields.size(); i++) {
        bool sel = (i == m_formSel);
        int h = 74;
        fillRect(x, y, m_w - 400, h - 12, sel ? kSel : kPanel);
        if (fields[i].save) {
            m_font.draw(m_r, x + 24, y + 14, "Save source", sel ? kDark : kGreen);
        } else {
            m_fontSmall.draw(m_r, x + 24, y + 6, fields[i].label, sel ? kDark : kDim);
            std::string shown = fields[i].pass ? std::string(fields[i].value.size(), '*')
                                               : fields[i].value;
            if (shown.empty()) shown = "(empty - press X to edit)";
            m_font.drawClipped(m_r, x + 24, y + 30, shown, sel ? kDark : kWhite, m_w - 460);
        }
        y += h;
    }
    m_fontSmall.draw(m_r, x, m_h - 60, "X: edit / save   O: back", kDim);
}

void App::renderBrowse()
{
    renderTopBar(m_playlist.title);

    if (m_playlist.categories.empty()) {
        m_font.draw(m_r, 120, 300, "No channels in this source.", kDim);
        return;
    }

    // Left: categories
    const int catX = 60, catY = 130, catW = 460;
    fillRect(catX, catY, catW, m_h - catY - 60, kPanel);
    m_fontSmall.draw(m_r, catX + 20, catY + 12, "CATEGORIES", kDim);

    int visibleCats = (m_h - catY - 120) / 56;
    int catStart = 0;
    if (m_catSel >= visibleCats) catStart = m_catSel - visibleCats + 1;

    int yy = catY + 48;
    for (int i = catStart; i < (int)m_playlist.categories.size() && i < catStart + visibleCats; i++) {
        bool sel = (i == m_catSel);
        if (sel) fillRect(catX + 8, yy - 4, catW - 16, 52, m_focusChannels ? kPanel2 : kSel);
        const Category &c = m_playlist.categories[i];
        std::string label = c.name + "  (" + std::to_string(c.channelIndices.size()) + ")";
        m_font.drawClipped(m_r, catX + 24, yy + 6, label,
                           (sel && !m_focusChannels) ? kDark : kWhite, catW - 60);
        yy += 56;
    }

    // Right: channels of selected category
    const int chX = catX + catW + 30, chY = 130;
    const int chW = m_w - chX - 60;
    fillRect(chX, chY, chW, m_h - chY - 60, kPanel);

    const Category &cat = m_playlist.categories[m_catSel];
    m_fontSmall.draw(m_r, chX + 20, chY + 12, "CHANNELS", kDim);

    int visibleCh = (m_h - chY - 120) / 72;
    if (m_chanSel < m_chanScroll) m_chanScroll = m_chanSel;
    if (m_chanSel >= m_chanScroll + visibleCh) m_chanScroll = m_chanSel - visibleCh + 1;

    int cy = chY + 48;
    for (int i = m_chanScroll; i < (int)cat.channelIndices.size() && i < m_chanScroll + visibleCh; i++) {
        int idx = cat.channelIndices[i];
        const Channel &ch = m_playlist.channels[idx];
        bool sel = (i == m_chanSel);
        if (sel) fillRect(chX + 8, cy - 4, chW - 16, 66, m_focusChannels ? kSel : kPanel2);

        // Thumbnail placeholder
        if (m_texPlaceholder) {
            SDL_Rect tr = { chX + 20, cy, 96, 54 };
            SDL_RenderCopy(m_r, m_texPlaceholder, nullptr, &tr);
        }
        Color tc = (sel && m_focusChannels) ? kDark : kWhite;
        m_font.drawClipped(m_r, chX + 132, cy + 4, ch.name, tc, chW - 300);

        // Codec badge
        Color badge = codec_is_supported(ch.codec) ? kGreen :
                      (ch.codec == CODEC_UNKNOWN ? kDim : kRed);
        std::string b = codecBadge(ch.codec);
        int bw = m_fontSmall.measure(b) + 24;
        fillRect(chX + chW - bw - 24, cy + 8, bw, 34, kDark);
        m_fontSmall.draw(m_r, chX + chW - bw - 12, cy + 12, b, badge);

        cy += 72;
    }

    m_fontSmall.draw(m_r, 60, m_h - 50,
        "D-pad: navigate   X: play   L1/R1: page   O: back", kDim);
}

void App::renderPlayer()
{
    SDL_SetRenderDrawColor(m_r, 0, 0, 0, 255);
    SDL_RenderClear(m_r);

    if (m_videoTex) {
        // Letterbox to preserve aspect ratio.
        float sw = (float)m_w / m_videoTexW;
        float sh = (float)m_h / m_videoTexH;
        float s = sw < sh ? sw : sh;
        int dw = (int)(m_videoTexW * s);
        int dh = (int)(m_videoTexH * s);
        SDL_Rect dst = { (m_w - dw) / 2, (m_h - dh) / 2, dw, dh };
        SDL_RenderCopy(m_r, m_videoTex, nullptr, &dst);
    } else {
        m_font.draw(m_r, m_w / 2 - 160, m_h / 2 - 20, "Buffering stream...", kWhite);
    }

    if (m_showOverlay) {
        fillRect(0, m_h - 130, m_w, 130, { 0, 0, 0, 170 });
        m_fontBig.drawClipped(m_r, 60, m_h - 118, m_nowPlaying, kWhite, m_w - 120);
        std::string state;
        switch (m_player.state()) {
            case VideoPlayer::State::Playing: state = "Playing"; break;
            case VideoPlayer::State::Paused:  state = "Paused";  break;
            case VideoPlayer::State::Opening: state = "Opening"; break;
            case VideoPlayer::State::Error:   state = "Error";   break;
            default: state = "Idle"; break;
        }
        std::string info = codec_name(m_nowCodec) + std::string("  |  ") + state +
                           "  |  X: pause   /\\: overlay   O: stop";
        m_font.draw(m_r, 60, m_h - 62, info, kDim);
    }
}

void App::renderMessage()
{
    fillRect(0, 0, m_w, m_h, { 0, 0, 0, 170 });
    const int pw = 900, ph = 300;
    const int px = (m_w - pw) / 2, py = (m_h - ph) / 2;
    fillRect(px, py, pw, ph, kPanel);
    frameRect(px, py, pw, ph, kAccent);
    m_fontBig.drawClipped(m_r, px + 40, py + 40, m_msgTitle, kWhite, pw - 80);
    m_font.drawClipped(m_r, px + 40, py + 120, m_msgBody, kDim, pw - 80);
    m_font.draw(m_r, px + 40, py + ph - 60, "X / O: dismiss", kDim);
}
