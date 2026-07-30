// ============================================================================
//  PS4 IPTV Player - entry point.
//
//  Sets up SDL2 (software renderer bound to the window surface, as used by the
//  OpenOrbis SDL build), opens the DualShock controller and runs the app loop.
// ============================================================================
#include <SDL2/SDL.h>
#include <cstdio>

#include "app.h"
#include "input.h"
#include "net_http.h"

#define FRAME_WIDTH  1920
#define FRAME_HEIGHT 1080

static Action buttonToAction(int button)
{
    switch (button) {
        case PAD_DPAD_UP:    return Action::Up;
        case PAD_DPAD_DOWN:  return Action::Down;
        case PAD_DPAD_LEFT:  return Action::Left;
        case PAD_DPAD_RIGHT: return Action::Right;
        case PAD_CROSS:      return Action::Confirm;
        case PAD_CIRCLE:     return Action::Back;
        case PAD_SQUARE:     return Action::Action1;
        case PAD_TRIANGLE:   return Action::Action2;
        case PAD_OPTIONS:    return Action::Menu;
        case PAD_L1:         return Action::PageUp;
        case PAD_R1:         return Action::PageDown;
        default:             return Action::None;
    }
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    setvbuf(stdout, NULL, _IONBF, 0);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("PS4 IPTV Player",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          FRAME_WIDTH, FRAME_HEIGHT, 0);
    if (!window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Surface  *surface  = SDL_GetWindowSurface(window);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    if (!renderer) {
        printf("SDL_CreateSoftwareRenderer failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Joystick *pad = nullptr;
    if (SDL_NumJoysticks() > 0)
        pad = SDL_JoystickOpen(0);

    // Bring up networking early so the first source load is responsive.
    http_global_init();

    App app;
    if (!app.init(renderer, window, FRAME_WIDTH, FRAME_HEIGHT)) {
        printf("App init failed\n");
        return 1;
    }

    // Analog stick edge-detection for menu navigation.
    const int DEADZONE = 20000;
    int lastAxisDir = 0; // 0 none, 1 up, 2 down

    Uint32 lastTicks = SDL_GetTicks();

    while (!app.wantsExit()) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_JOYBUTTONDOWN:
                    app.handleAction(buttonToAction(ev.jbutton.button));
                    break;
                case SDL_JOYAXISMOTION:
                    if (ev.jaxis.axis == 1) { // left stick Y
                        int dir = 0;
                        if (ev.jaxis.value < -DEADZONE) dir = 1;
                        else if (ev.jaxis.value > DEADZONE) dir = 2;
                        if (dir != lastAxisDir) {
                            lastAxisDir = dir;
                            if (dir == 1) app.handleAction(Action::Up);
                            else if (dir == 2) app.handleAction(Action::Down);
                        }
                    }
                    break;
                case SDL_QUIT:
                    return 0;
                default: break;
            }
        }

        app.update();
        app.render();
        app.present();

        // Cap to ~60 FPS.
        Uint32 now = SDL_GetTicks();
        Uint32 dt  = now - lastTicks;
        if (dt < 16) SDL_Delay(16 - dt);
        lastTicks = SDL_GetTicks();
    }

    app.shutdown();
    if (pad) SDL_JoystickClose(pad);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
