// ============================================================================
//  syskeyboard.h - PS4 system on-screen keyboard (SceImeDialog).
//
//  Wraps the native IME dialog so text entry uses the console's own keyboard
//  (with full language / emoji / controller + USB-keyboard support). Falls back
//  to the app's drawn keyboard when the dialog is unavailable.
// ============================================================================
#ifndef PS4_IPTV_SYSKEYBOARD_H
#define PS4_IPTV_SYSKEYBOARD_H

#include <string>

// Initialise the common-dialog subsystem. Safe to call more than once.
// Returns true when the system keyboard can be used.
bool sys_keyboard_init();

bool sys_keyboard_available();

// Show the system keyboard modally and block until the user confirms/cancels.
// Returns true if the user accepted; `out` then holds the entered text.
bool sys_keyboard_prompt(const std::string &title,
                         const std::string &initial,
                         bool password,
                         std::string &out);

#endif // PS4_IPTV_SYSKEYBOARD_H
