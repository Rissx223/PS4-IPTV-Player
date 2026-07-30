// ============================================================================
//  syskeyboard.cpp - SceImeDialog wrapper (PS4 system keyboard).
// ============================================================================
#include "syskeyboard.h"

#include <cstdint>
#include <cstring>
#include <ctime>
#include <vector>

#include <orbis/CommonDialog.h>
#include <orbis/ImeDialog.h>
#include <orbis/UserService.h>

// IME text buffers are UTF-16. On this toolchain wchar_t is 32-bit, so we keep
// the buffers as uint16_t and cast only at the API boundary.
namespace {

const size_t kMaxText = 512;

bool     g_ready  = false;
int32_t  g_userId = 0;

void utf8ToU16(const std::string &s, uint16_t *out, size_t cap)
{
    size_t oi = 0;
    const unsigned char *p = (const unsigned char *)s.c_str();
    while (*p && oi + 1 < cap) {
        unsigned char c = *p;
        uint32_t cp;
        if (c < 0x80)                         { cp = c;                              p += 1; }
        else if ((c >> 5) == 0x6 && p[1])     { cp = ((c & 0x1F) << 6) | (p[1] & 0x3F); p += 2; }
        else if ((c >> 4) == 0xE && p[1] && p[2]) {
            cp = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3;
        } else                                { cp = '?';                            p += 1; }
        if (cp > 0xFFFF) cp = '?';
        out[oi++] = (uint16_t)cp;
    }
    out[oi] = 0;
}

std::string u16ToUtf8(const uint16_t *in)
{
    std::string out;
    for (size_t i = 0; in[i] != 0; i++) {
        uint32_t cp = in[i];
        if (cp < 0x80)        out.push_back((char)cp);
        else if (cp < 0x800) {
            out.push_back((char)(0xC0 | (cp >> 6)));
            out.push_back((char)(0x80 | (cp & 0x3F)));
        } else {
            out.push_back((char)(0xE0 | (cp >> 12)));
            out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

} // namespace

bool sys_keyboard_init()
{
    if (g_ready) return true;

    sceUserServiceInitialize(nullptr);
    if (sceUserServiceGetInitialUser(&g_userId) != 0)
        g_userId = 0;

    sceCommonDialogInitialize();  // idempotent; ignore "already initialised"
    g_ready = true;
    return true;
}

bool sys_keyboard_available()
{
    return g_ready;
}

bool sys_keyboard_prompt(const std::string &title, const std::string &initial,
                         bool password, std::string &out)
{
    if (!g_ready)
        return false;

    std::vector<uint16_t> textBuf(kMaxText, 0);
    uint16_t titleBuf[128] = { 0 };
    uint16_t placeBuf[64]  = { 0 };

    utf8ToU16(initial, textBuf.data(), kMaxText);
    utf8ToU16(title.empty() ? "Enter text" : title, titleBuf, 128);
    utf8ToU16(password ? "password" : "", placeBuf, 64);

    OrbisImeDialogSetting p;
    memset(&p, 0, sizeof(p));
    p.userId              = (uint32_t)g_userId;
    p.type                = ORBIS_TYPE_DEFAULT;
    p.supportedLanguages  = 0;
    p.enterLabel          = ORBIS_BUTTON_LABEL_DEFAULT;
    p.inputMethod         = ORBIS__DEFAULT;
    p.filter              = nullptr;
    p.option              = password ? 0x1u : 0x0u;   // password mask bit
    p.maxTextLength       = (uint32_t)(kMaxText - 1);
    p.inputTextBuffer     = (wchar_t *)textBuf.data();
    p.posx                = 0.0f;
    p.posy                = 0.0f;
    p.horizontalAlignment = ORBIS_H_CENTER;
    p.verticalAlignment   = ORBIS_V_CENTER;
    p.placeholder         = (const wchar_t *)placeBuf;
    p.title               = (const wchar_t *)titleBuf;

    if (sceImeDialogInit(&p, nullptr) != 0)
        return false;

    struct timespec ts = { 0, 10 * 1000 * 1000 };
    OrbisDialogStatus st;
    while ((st = sceImeDialogGetStatus()) == ORBIS_DIALOG_STATUS_RUNNING)
        nanosleep(&ts, nullptr);

    bool accepted = false;
    if (st == ORBIS_DIALOG_STATUS_STOPPED) {
        OrbisDialogResult res;
        memset(&res, 0, sizeof(res));
        sceImeDialogGetResult(&res);
        if (res.endstatus == ORBIS_DIALOG_OK) {
            out = u16ToUtf8(textBuf.data());
            accepted = true;
        }
    }
    sceImeDialogTerm();
    return accepted;
}
