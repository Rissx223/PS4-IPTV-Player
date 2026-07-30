#include "player.h"

#include <cstdlib>
#include <cstring>

#include <orbis/Sysmodule.h>

// ---- AvPlayer memory callbacks --------------------------------------------
extern "C" {

static void *iptv_av_allocate(void *arg, uint32_t alignment, uint32_t size)
{
    (void)arg;
    void *ptr = nullptr;
    if (alignment < sizeof(void *)) alignment = sizeof(void *);
    // Round size up to a multiple of the alignment (posix_memalign requirement).
    size_t asz = (size + alignment - 1) & ~((size_t)alignment - 1);
    if (posix_memalign(&ptr, alignment, asz) != 0)
        return nullptr;
    return ptr;
}

static void iptv_av_deallocate(void *arg, void *ptr)
{
    (void)arg;
    free(ptr);
}

static void *iptv_av_allocate_texture(void *arg, uint32_t alignment, uint32_t size)
{
    return iptv_av_allocate(arg, alignment, size);
}

static void iptv_av_deallocate_texture(void *arg, void *ptr)
{
    (void)arg;
    free(ptr);
}

} // extern "C"

bool VideoPlayer::globalInit()
{
    if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_AV_PLAYER) < 0)
        return false;
    return true;
}

VideoPlayer::~VideoPlayer()
{
    stop();
}

bool VideoPlayer::open(const std::string &url, StreamCodec codec)
{
    stop();
    m_error.clear();
    m_hasFrame = false;

    if (!codec_is_supported(codec) && codec != CODEC_UNKNOWN) {
        m_state = State::Error;
        m_error = codec_support_note(codec);
        return false;
    }

    SceAvPlayerInitData init;
    memset(&init, 0, sizeof(init));
    init.memoryReplacement.allocate          = iptv_av_allocate;
    init.memoryReplacement.deallocate        = iptv_av_deallocate;
    init.memoryReplacement.allocateTexture   = iptv_av_allocate_texture;
    init.memoryReplacement.deallocateTexture = iptv_av_deallocate_texture;
    init.debugLevel                          = SCE_AVPLAYER_DBG_NONE;
    init.basePriority                        = 160;
    init.numOutputVideoFrameBuffers          = 4;
    init.autoStart                           = 1;
    init.defaultLanguage                     = "en";

    m_handle = sceAvPlayerInit(&init);
    if (!m_handle) {
        m_state = State::Error;
        m_error = "Failed to initialise AvPlayer";
        return false;
    }

    // Let AvPlayer sniff the container; HLS gets an explicit hint.
    int rc;
    bool isHls = url.find(".m3u8") != std::string::npos;
    if (isHls)
        rc = sceAvPlayerAddSourceEx(m_handle, SCE_AVPLAYER_SOURCE_TYPE_HLS, url.c_str());
    else
        rc = sceAvPlayerAddSource(m_handle, url.c_str());

    if (rc < 0) {
        m_error = "Failed to open stream source";
        m_state = State::Error;
        sceAvPlayerClose(m_handle);
        m_handle = nullptr;
        return false;
    }

    m_state = State::Opening;
    return true;
}

bool VideoPlayer::update()
{
    if (!m_handle || m_state == State::Error)
        return false;

    if (!sceAvPlayerIsActive(m_handle)) {
        // Still starting up, or stream ended.
        if (m_state == State::Playing)
            m_state = State::Idle;
        return false;
    }

    SceAvPlayerFrameInfoEx frame;
    memset(&frame, 0, sizeof(frame));

    if (sceAvPlayerGetVideoDataEx(m_handle, &frame) != 0 || frame.pData == nullptr)
        return false;

    m_state = State::Playing;

    int w     = (int)frame.details.video.width;
    int h     = (int)frame.details.video.height;
    int pitch = (int)frame.details.video.pitch;
    if (pitch <= 0) pitch = w;
    if (w <= 0 || h <= 0)
        return false;

    const uint8_t *luma   = (const uint8_t *)frame.pData;
    const uint8_t *chroma = luma + (size_t)pitch * h;

    nv12ToArgb(luma, chroma, w, h, pitch);
    m_hasFrame = true;
    return true;
}

// BT.601 limited-range NV12 -> ARGB8888.
void VideoPlayer::nv12ToArgb(const uint8_t *luma, const uint8_t *chroma,
                             int w, int h, int pitch)
{
    if (w != m_width || h != m_height) {
        m_width  = w;
        m_height = h;
        m_argb.assign((size_t)w * h, 0xFF000000u);
    }

    uint32_t *dst = m_argb.data();

    for (int y = 0; y < h; y++) {
        const uint8_t *yrow = luma + (size_t)y * pitch;
        const uint8_t *crow = chroma + (size_t)(y >> 1) * pitch;
        uint32_t *drow = dst + (size_t)y * w;

        for (int x = 0; x < w; x++) {
            int Y = yrow[x];
            int U = crow[(x & ~1)]     - 128;
            int V = crow[(x & ~1) + 1] - 128;

            int c = Y - 16;
            int r = (298 * c + 409 * V + 128) >> 8;
            int g = (298 * c - 100 * U - 208 * V + 128) >> 8;
            int b = (298 * c + 516 * U + 128) >> 8;

            if (r < 0) r = 0; else if (r > 255) r = 255;
            if (g < 0) g = 0; else if (g > 255) g = 255;
            if (b < 0) b = 0; else if (b > 255) b = 255;

            drow[x] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}

void VideoPlayer::togglePause()
{
    if (!m_handle) return;
    if (m_state == State::Paused) {
        sceAvPlayerResume(m_handle);
        m_state = State::Playing;
    } else if (m_state == State::Playing) {
        sceAvPlayerPause(m_handle);
        m_state = State::Paused;
    }
}

void VideoPlayer::stop()
{
    if (m_handle) {
        sceAvPlayerStop(m_handle);
        sceAvPlayerClose(m_handle);
        m_handle = nullptr;
    }
    m_state    = State::Idle;
    m_hasFrame = false;
}
