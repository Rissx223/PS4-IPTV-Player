// ============================================================================
//  app.h - Application state machine, screens and rendering.
// ============================================================================
#ifndef PS4_IPTV_APP_H
#define PS4_IPTV_APP_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>

#include "model.h"
#include "input.h"
#include "text.h"
#include "keyboard.h"
#include "player.h"

class App {
public:
    bool init(SDL_Renderer *renderer, SDL_Window *window, int width, int height);
    void shutdown();
    void present();   // push the rendered surface to the screen

    void handleAction(Action a);
    void update();          // advance video / async work
    void render();

    bool wantsExit() const { return m_exit; }

private:
    enum class Screen {
        Home,        // list of saved sources + add actions
        AddType,     // choose new source kind
        Form,        // edit fields of a draft source
        Browse,      // categories + channels of the loaded playlist
        PlayerView,  // fullscreen video
        Message      // transient info / error overlay
    };

    // ---- screen handlers ----
    void handleHome(Action a);
    void handleAddType(Action a);
    void handleForm(Action a);
    void handleBrowse(Action a);
    void handlePlayer(Action a);

    void renderHome();
    void renderAddType();
    void renderForm();
    void renderBrowse();
    void renderPlayer();
    void renderTopBar(const std::string &subtitle);
    void renderMessage();

    // ---- helpers ----
    void loadSource(int index);
    void beginAddSource(SourceKind kind);
    void enterFormFieldEditor();
    void saveDraftSource();
    void showMessage(const std::string &title, const std::string &body);
    void startPlayback(int channelIndex);

    SDL_Texture *loadImage(const char *path);
    void fillRect(int x, int y, int w, int h, Color c);
    void frameRect(int x, int y, int w, int h, Color c);

    // ---- state ----
    SDL_Renderer *m_r = nullptr;
    SDL_Window   *m_window = nullptr;
    int           m_w = 1920;
    int           m_h = 1080;
    bool          m_exit = false;

    Screen        m_screen = Screen::Home;
    Screen        m_returnScreen = Screen::Home;

    Font          m_font;      // body
    Font          m_fontSmall; // captions
    Font          m_fontBig;   // headings

    // Images
    SDL_Texture  *m_texLogo        = nullptr;
    SDL_Texture  *m_texBackground  = nullptr;
    SDL_Texture  *m_texPlaceholder = nullptr;
    SDL_Texture  *m_texXtream      = nullptr;
    SDL_Texture  *m_texM3u         = nullptr;
    SDL_Texture  *m_texLocal       = nullptr;

    // Sources / playlist
    std::vector<SourceProfile> m_sources;
    int                        m_homeSel = 0;   // selected row on home
    Playlist                   m_playlist;

    // Browse
    int m_catSel = 0;
    int m_chanSel = 0;
    int m_chanScroll = 0;
    bool m_focusChannels = false;

    // Add-type
    int m_addTypeSel = 0;

    // Form editing
    SourceProfile m_draft;
    int           m_editIndex = -1;   // index in m_sources when editing existing
    int           m_formSel = 0;
    bool          m_keyboardActive = false;
    OnScreenKeyboard m_keyboard;
    int           m_keyboardField = -1;

    // Message overlay
    std::string m_msgTitle;
    std::string m_msgBody;

    // Playback
    VideoPlayer m_player;
    SDL_Texture *m_videoTex = nullptr;
    int          m_videoTexW = 0;
    int          m_videoTexH = 0;
    std::string  m_nowPlaying;
    StreamCodec  m_nowCodec = CODEC_UNKNOWN;
    bool         m_showOverlay = true;
};

#endif // PS4_IPTV_APP_H
