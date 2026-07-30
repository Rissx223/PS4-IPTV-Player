// ============================================================================
//  player.h - Video playback layer.
//
//  Wraps libSceAvPlayer, which demuxes the container (TS/MP4/HLS over
//  HTTP/HTTPS or local files) and drives the PS4 fixed-function video decoder.
//  Decoded frames arrive as NV12 and are colour-converted to ARGB8888 for the
//  UI's SDL renderer. Only the codecs the hardware can decode (H.264 / H.265)
//  will actually produce frames; other codecs are rejected up-front via the
//  codec abstraction so the UI can show a clear message.
// ============================================================================
#ifndef PS4_IPTV_PLAYER_H
#define PS4_IPTV_PLAYER_H

#include <cstdint>
#include <string>
#include <vector>
#include "codec.h"
#include "orbis_avplayer.h"

class VideoPlayer {
public:
    enum class State { Idle, Opening, Playing, Paused, Error };

    VideoPlayer() = default;
    ~VideoPlayer();

    // One-time module load. Returns true on success.
    static bool globalInit();

    // Begin playback of a URL previously classified with `codec`. Returns
    // false immediately (State::Error) for codecs with no PS4 hardware path.
    bool open(const std::string &url, StreamCodec codec);

    // Pull the newest decoded frame. When a frame is ready the internal ARGB
    // buffer is refreshed and true is returned. Call once per UI frame.
    bool update();

    void togglePause();
    void stop();

    State        state() const { return m_state; }
    const char  *errorText() const { return m_error.c_str(); }

    // Latest decoded frame as tightly-packed ARGB8888 (width*height pixels).
    const uint32_t *framePixels() const { return m_argb.empty() ? nullptr : m_argb.data(); }
    int frameWidth()  const { return m_width; }
    int frameHeight() const { return m_height; }
    bool hasFrame()   const { return m_hasFrame; }

private:
    void nv12ToArgb(const uint8_t *luma, const uint8_t *chroma,
                    int w, int h, int pitch);

    SceAvPlayerHandle     m_handle  = nullptr;
    State                 m_state   = State::Idle;
    std::string           m_error;
    std::vector<uint32_t> m_argb;
    int                   m_width   = 0;
    int                   m_height  = 0;
    bool                  m_hasFrame = false;
};

#endif // PS4_IPTV_PLAYER_H
