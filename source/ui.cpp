#include "app.h"

#include <SDL2/SDL_image.h>
#include <cctype>

#include "config.h"
#include "playlist.h"
#include "syskeyboard.h"
#include "xtream.h"

// Sentinel field id used to route keyboard input to the search box.
static const int kSearchFieldId = -100;

namespace {

struct FormField {
    std::string  label;
    std::string *value;
    bool         password;
    bool         isSave;
};

const char *kindName(SourceKind k)
{
    switch (k) {
        case SourceKind::Xtream:    return "Xtream API";
        case SourceKind::M3uUrl:    return "M3U URL";
        case SourceKind::LocalFile: return "Local Playlist";
    }
    return "Source";
}

} // namespace

// Build the field list for the current draft source.
static std::vector<FormField> buildFields(SourceProfile &d)
{
    std::vector<FormField> f;
    f.push_back({ "Name", &d.name, false, false });
    switch (d.kind) {
        case SourceKind::Xtream:
            f.push_back({ "Host (http://panel:port)", &d.host, false, false });
            f.push_back({ "Username", &d.username, false, false });
            f.push_back({ "Password", &d.password, true, false });
            break;
        case SourceKind::M3uUrl:
            f.push_back({ "Playlist URL", &d.url, false, false });
            break;
        case SourceKind::LocalFile:
            f.push_back({ "File path (/data/...)", &d.url, false, false });
            break;
    }
    f.push_back({ "Save", nullptr, false, true });
    return f;
}

// ===========================================================================
//  Lifecycle
// ===========================================================================
bool App::init(SDL_Renderer *renderer, SDL_Window *window, int width, int height)
{
    m_r = renderer;
    m_window = window;
    m_w = width;
    m_h = height;

    if (!text_global_init())
        return false;

    m_fontBig.load("/app0/assets/fonts/Gontserrat-Regular.ttf", 40);
    m_font.load("/app0/assets/fonts/Gontserrat-Regular.ttf", 26);
    m_fontSmall.load("/app0/assets/fonts/Gontserrat-Regular.ttf", 20);

    IMG_Init(IMG_INIT_PNG);
    m_texLogo        = loadImage("/app0/assets/images/logo.png");
    m_texBackground  = loadImage("/app0/assets/images/background.png");
    m_texPlaceholder = loadImage("/app0/assets/images/placeholder.png");
    m_texXtream      = loadImage("/app0/assets/images/icon_xtream.png");
    m_texM3u         = loadImage("/app0/assets/images/icon_m3u.png");
    m_texLocal       = loadImage("/app0/assets/images/icon_local.png");

    VideoPlayer::globalInit();

    m_useSysKeyboard = sys_keyboard_init();

    config_ensure_dir();
    m_sources   = config_load_sources();
    m_favorites = config_load_favorites();

    m_screen = Screen::Home;
    return true;
}

void App::shutdown()
{
    m_player.stop();
    if (m_videoTex) { SDL_DestroyTexture(m_videoTex); m_videoTex = nullptr; }
    m_font.unload();
    m_fontSmall.unload();
    m_fontBig.unload();
}

void App::present()
{
    // The OpenOrbis SDL build uses a software renderer bound to the window
    // surface, so frames are shown by updating that surface.
    if (m_window)
        SDL_UpdateWindowSurface(m_window);
}

SDL_Texture *App::loadImage(const char *path)
{
    SDL_Surface *s = IMG_Load(path);
    if (!s) return nullptr;
    SDL_Texture *t = SDL_CreateTextureFromSurface(m_r, s);
    SDL_FreeSurface(s);
    if (t) SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    return t;
}

// ===========================================================================
//  Small drawing helpers
// ===========================================================================
void App::fillRect(int x, int y, int w, int h, Color c)
{
    SDL_SetRenderDrawBlendMode(m_r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_r, c.r, c.g, c.b, c.a);
    SDL_Rect rc = { x, y, w, h };
    SDL_RenderFillRect(m_r, &rc);
}

void App::frameRect(int x, int y, int w, int h, Color c)
{
    SDL_SetRenderDrawColor(m_r, c.r, c.g, c.b, c.a);
    SDL_Rect rc = { x, y, w, h };
    SDL_RenderDrawRect(m_r, &rc);
}

// ===========================================================================
//  Input dispatch
// ===========================================================================
void App::handleAction(Action a)
{
    if (a == Action::None) return;

    if (m_screen == Screen::Message) {
        if (a == Action::Confirm || a == Action::Back)
            m_screen = m_returnScreen;
        return;
    }

    if (m_keyboardActive) {
        bool done = false, accepted = false;
        m_keyboard.handle(a, done, accepted);
        if (done) {
            m_keyboardActive = false;
            if (accepted) {
                if (m_keyboardField == kSearchFieldId) {
                    m_searchQuery = m_keyboard.text();
                    rebuildView();
                } else if (m_keyboardField >= 0) {
                    auto fields = buildFields(m_draft);
                    if (m_keyboardField < (int)fields.size() && fields[m_keyboardField].value)
                        *fields[m_keyboardField].value = m_keyboard.text();
                }
            }
        }
        return;
    }

    switch (m_screen) {
        case Screen::Home:       handleHome(a); break;
        case Screen::AddType:    handleAddType(a); break;
        case Screen::Form:       handleForm(a); break;
        case Screen::Browse:     handleBrowse(a); break;
        case Screen::PlayerView: handlePlayer(a); break;
        default: break;
    }
}

// ===========================================================================
//  Home screen
// ===========================================================================
void App::handleHome(Action a)
{
    int total = (int)m_sources.size() + 3; // + Add Xtream / Add M3U / Add Local

    switch (a) {
        case Action::Up:    m_homeSel = (m_homeSel + total - 1) % total; break;
        case Action::Down:  m_homeSel = (m_homeSel + 1) % total; break;
        case Action::Confirm: {
            if (m_homeSel < (int)m_sources.size()) {
                loadSource(m_homeSel);
            } else {
                int add = m_homeSel - (int)m_sources.size();
                if (add == 0) beginAddSource(SourceKind::Xtream);
                else if (add == 1) beginAddSource(SourceKind::M3uUrl);
                else beginAddSource(SourceKind::LocalFile);
            }
            break;
        }
        case Action::Action2: // Triangle = delete selected source
            if (m_homeSel < (int)m_sources.size()) {
                m_sources.erase(m_sources.begin() + m_homeSel);
                config_save_sources(m_sources);
                if (m_homeSel > 0) m_homeSel--;
            }
            break;
        case Action::Action1: // Square = edit selected source
            if (m_homeSel < (int)m_sources.size()) {
                m_draft = m_sources[m_homeSel];
                m_editIndex = m_homeSel;
                m_formSel = 0;
                m_screen = Screen::Form;
            }
            break;
        case Action::Back:
            m_exit = true;
            break;
        default: break;
    }
}

void App::loadSource(int index)
{
    if (index < 0 || index >= (int)m_sources.size()) return;
    showMessage("Loading", "Contacting source, please wait...");
    render();  // draw the loading overlay immediately
    present();

    LoadResult res = playlist_load(m_sources[index], m_playlist);
    if (!res.ok) {
        showMessage("Load failed", res.message);
        return;
    }
    m_currentSource = index;

    // Re-apply saved favorites to the freshly loaded channels.
    for (auto &ch : m_playlist.channels)
        ch.favorite = (m_favorites.count(ch.url) > 0);

    m_searchQuery.clear();
    rebuildView();

    m_catSel = 0;
    m_chanSel = 0;
    m_chanScroll = 0;
    m_focusChannels = false;
    m_screen = Screen::Browse;
}

// Compose the browse view: optional Favorites and Search categories first,
// then the playlist's real categories.
void App::rebuildView()
{
    m_viewCats.clear();

    Category fav;
    fav.id = "__fav__";
    fav.name = "\xE2\x98\x85 Favorites"; // ★ Favorites
    for (int i = 0; i < (int)m_playlist.channels.size(); i++)
        if (m_playlist.channels[i].favorite)
            fav.channelIndices.push_back(i);
    if (!fav.channelIndices.empty())
        m_viewCats.push_back(std::move(fav));

    if (!m_searchQuery.empty()) {
        std::string q = m_searchQuery;
        for (auto &c : q) c = (char)tolower((unsigned char)c);
        Category sr;
        sr.id = "__search__";
        sr.name = "Search: " + m_searchQuery;
        for (int i = 0; i < (int)m_playlist.channels.size(); i++) {
            std::string n = m_playlist.channels[i].name;
            for (auto &c : n) c = (char)tolower((unsigned char)c);
            if (n.find(q) != std::string::npos)
                sr.channelIndices.push_back(i);
        }
        m_viewCats.push_back(std::move(sr));
    }

    for (const auto &c : m_playlist.categories)
        m_viewCats.push_back(c);
}

void App::toggleFavorite(int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= (int)m_playlist.channels.size())
        return;
    Channel &ch = m_playlist.channels[channelIndex];
    ch.favorite = !ch.favorite;
    if (ch.favorite) m_favorites.insert(ch.url);
    else             m_favorites.erase(ch.url);
    config_save_favorites(m_favorites);
    rebuildView();
    if (m_catSel >= (int)m_viewCats.size()) m_catSel = 0;
}

void App::beginSearch()
{
    promptText("Search channels", m_searchQuery, false, kSearchFieldId);
}

// ===========================================================================
//  Add-source type chooser
// ===========================================================================
void App::beginAddSource(SourceKind kind)
{
    m_draft = SourceProfile();
    m_draft.kind = kind;
    m_editIndex = -1;
    m_formSel = 0;
    m_screen = Screen::Form;
}

void App::handleAddType(Action a)
{
    switch (a) {
        case Action::Up:    m_addTypeSel = (m_addTypeSel + 2) % 3; break;
        case Action::Down:  m_addTypeSel = (m_addTypeSel + 1) % 3; break;
        case Action::Confirm:
            beginAddSource(m_addTypeSel == 0 ? SourceKind::Xtream :
                           m_addTypeSel == 1 ? SourceKind::M3uUrl : SourceKind::LocalFile);
            break;
        case Action::Back: m_screen = Screen::Home; break;
        default: break;
    }
}

// ===========================================================================
//  Source form
// ===========================================================================
void App::handleForm(Action a)
{
    auto fields = buildFields(m_draft);
    int n = (int)fields.size();

    switch (a) {
        case Action::Up:    m_formSel = (m_formSel + n - 1) % n; break;
        case Action::Down:  m_formSel = (m_formSel + 1) % n; break;
        case Action::Confirm: {
            if (fields[m_formSel].isSave) {
                saveDraftSource();
            } else {
                enterFormFieldEditor();
            }
            break;
        }
        case Action::Back: m_screen = Screen::Home; break;
        default: break;
    }
}

void App::enterFormFieldEditor()
{
    auto fields = buildFields(m_draft);
    if (m_formSel >= (int)fields.size() || !fields[m_formSel].value)
        return;
    promptText(fields[m_formSel].label, *fields[m_formSel].value,
               fields[m_formSel].password, m_formSel);
}

// Prefer the PS4 system keyboard (modal/blocking); fall back to the drawn one.
void App::promptText(const std::string &title, const std::string &initial,
                     bool password, int fieldId)
{
    if (m_useSysKeyboard && sys_keyboard_available()) {
        std::string result;
        if (sys_keyboard_prompt(title, initial, password, result)) {
            if (fieldId == kSearchFieldId) {
                m_searchQuery = result;
                rebuildView();
            } else if (fieldId >= 0) {
                auto fields = buildFields(m_draft);
                if (fieldId < (int)fields.size() && fields[fieldId].value)
                    *fields[fieldId].value = result;
            }
        }
        return;
    }
    // Fallback: on-screen drawn keyboard.
    m_keyboardField = fieldId;
    m_keyboardActive = true;
    m_keyboard.begin(title, initial, password);
}

void App::saveDraftSource()
{
    if (m_draft.name.empty())
        m_draft.name = kindName(m_draft.kind);

    if (m_editIndex >= 0 && m_editIndex < (int)m_sources.size())
        m_sources[m_editIndex] = m_draft;
    else
        m_sources.push_back(m_draft);

    config_save_sources(m_sources);
    m_homeSel = 0;
    m_screen = Screen::Home;
}

// ===========================================================================
//  Browse screen
// ===========================================================================
void App::handleBrowse(Action a)
{
    // Search is available from anywhere in the browser via Options.
    if (a == Action::Menu) { beginSearch(); return; }

    if (m_viewCats.empty()) {
        if (a == Action::Back) m_screen = Screen::Home;
        return;
    }
    if (m_catSel >= (int)m_viewCats.size()) m_catSel = 0;

    if (!m_focusChannels) {
        int nc = (int)m_viewCats.size();
        switch (a) {
            case Action::Up:    m_catSel = (m_catSel + nc - 1) % nc; m_chanSel = 0; m_chanScroll = 0; break;
            case Action::Down:  m_catSel = (m_catSel + 1) % nc; m_chanSel = 0; m_chanScroll = 0; break;
            case Action::Right:
            case Action::Confirm: m_focusChannels = true; break;
            case Action::Back:  m_screen = Screen::Home; break;
            default: break;
        }
    } else {
        const Category &cat = m_viewCats[m_catSel];
        int cnt = (int)cat.channelIndices.size();
        if (cnt == 0) { m_focusChannels = false; return; }
        if (m_chanSel >= cnt) m_chanSel = cnt - 1;
        switch (a) {
            case Action::Up:    m_chanSel = (m_chanSel + cnt - 1) % cnt; break;
            case Action::Down:  m_chanSel = (m_chanSel + 1) % cnt; break;
            case Action::PageUp:   m_chanSel = (m_chanSel - 8 < 0) ? 0 : m_chanSel - 8; break;
            case Action::PageDown: m_chanSel = (m_chanSel + 8 >= cnt) ? cnt - 1 : m_chanSel + 8; break;
            case Action::Left:  m_focusChannels = false; break;
            case Action::Back:  m_focusChannels = false; break;
            case Action::Action1: // Square = toggle favorite
                toggleFavorite(cat.channelIndices[m_chanSel]);
                break;
            case Action::Confirm:
                startPlayback(cat.channelIndices[m_chanSel]);
                break;
            default: break;
        }
    }
}

// ===========================================================================
//  Playback
// ===========================================================================
void App::startPlayback(int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= (int)m_playlist.channels.size())
        return;
    Channel &ch = m_playlist.channels[channelIndex];

    m_nowPlaying = ch.name;
    m_nowCodec   = ch.codec;
    m_showOverlay = true;

    // Best-effort EPG (now/next) for Xtream live channels.
    m_epgNow.clear();
    m_epgNext.clear();
    if (m_playlist.kind == SourceKind::Xtream &&
        m_currentSource >= 0 && m_currentSource < (int)m_sources.size() &&
        ch.url.find("/live/") != std::string::npos) {
        if (xtream_short_epg(m_sources[m_currentSource], ch.id, m_epgNow, m_epgNext)) {
            ch.epgNow = m_epgNow;
            ch.epgNext = m_epgNext;
        }
    }

    if (!codec_is_supported(ch.codec) && ch.codec != CODEC_UNKNOWN) {
        showMessage(std::string("Unsupported codec: ") + codec_name(ch.codec),
                    codec_support_note(ch.codec));
        m_returnScreen = Screen::Browse;
        return;
    }

    if (!m_player.open(ch.url, ch.codec)) {
        showMessage("Playback error", m_player.errorText());
        m_returnScreen = Screen::Browse;
        return;
    }
    m_screen = Screen::PlayerView;
}

void App::handlePlayer(Action a)
{
    switch (a) {
        case Action::Back:
            m_player.stop();
            m_screen = Screen::Browse;
            break;
        case Action::Confirm:
            m_player.togglePause();
            break;
        case Action::Action2: // Triangle toggles overlay
            m_showOverlay = !m_showOverlay;
            break;
        default: break;
    }
}

void App::update()
{
    if (m_screen != Screen::PlayerView)
        return;

    if (m_player.update() && m_player.hasFrame()) {
        int w = m_player.frameWidth();
        int h = m_player.frameHeight();
        if (!m_videoTex || w != m_videoTexW || h != m_videoTexH) {
            if (m_videoTex) SDL_DestroyTexture(m_videoTex);
            m_videoTex = SDL_CreateTexture(m_r, SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING, w, h);
            m_videoTexW = w;
            m_videoTexH = h;
        }
        if (m_videoTex)
            SDL_UpdateTexture(m_videoTex, nullptr, m_player.framePixels(), w * 4);
    }
}

// ===========================================================================
//  Message overlay
// ===========================================================================
void App::showMessage(const std::string &title, const std::string &body)
{
    m_msgTitle = title;
    m_msgBody = body;
    m_returnScreen = (m_screen == Screen::Message) ? m_returnScreen : m_screen;
    m_screen = Screen::Message;
}
