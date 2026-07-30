// ============================================================================
//  input.h - DualShock 4 button indices as seen through SDL2's joystick API
//            on the OpenOrbis SDL build, plus a logical action enum.
// ============================================================================
#ifndef PS4_IPTV_INPUT_H
#define PS4_IPTV_INPUT_H

enum PadButton {
    PAD_CROSS     = 0,
    PAD_CIRCLE    = 1,
    PAD_SQUARE    = 2,
    PAD_TRIANGLE  = 3,
    PAD_L1        = 4,
    PAD_R1        = 5,
    PAD_OPTIONS   = 9,
    PAD_L3        = 11,
    PAD_R3        = 12,
    PAD_DPAD_UP   = 13,
    PAD_DPAD_DOWN = 14,
    PAD_DPAD_LEFT = 15,
    PAD_DPAD_RIGHT= 16,
    PAD_TOUCHPAD  = 17,
    PAD_L2        = 18,
    PAD_R2        = 19
};

enum class Action {
    None,
    Up, Down, Left, Right,
    Confirm,    // Cross
    Back,       // Circle
    Action1,    // Square
    Action2,    // Triangle
    Menu,       // Options
    PageUp,     // L1
    PageDown    // R1
};

#endif // PS4_IPTV_INPUT_H
