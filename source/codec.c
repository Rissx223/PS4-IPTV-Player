#include "codec.h"
#include <string.h>
#include <ctype.h>

const char *codec_name(StreamCodec c)
{
    switch (c) {
        case CODEC_H264: return "H.264";
        case CODEC_H265: return "H.265";
        case CODEC_AV1:  return "AV1";
        case CODEC_VP9:  return "VP9";
        case CODEC_VVC:  return "VVC";
        default:         return "Unknown";
    }
}

int codec_is_supported(StreamCodec c)
{
    if (c == CODEC_H264 || c == CODEC_H265)
        return 1;
#ifdef USE_FFMPEG
    /* Software (FFmpeg) backend can decode these, albeit not in real time. */
    if (c == CODEC_AV1 || c == CODEC_VP9 || c == CODEC_VVC)
        return 1;
#endif
    return 0;
}

CodecDecodePath codec_decode_path(StreamCodec c)
{
    if (c == CODEC_H264 || c == CODEC_H265)
        return CODEC_PATH_HARDWARE;
#ifdef USE_FFMPEG
    if (c == CODEC_AV1 || c == CODEC_VP9 || c == CODEC_VVC)
        return CODEC_PATH_SOFTWARE;
#endif
    return CODEC_PATH_NONE;
}

const char *codec_support_note(StreamCodec c)
{
    switch (c) {
        case CODEC_H264: return "Hardware decoded (AVC).";
        case CODEC_H265: return "Hardware decoded (HEVC).";
#ifdef USE_FFMPEG
        case CODEC_AV1:  return "AV1 decoded in software (FFmpeg) - may not be real time.";
        case CODEC_VP9:  return "VP9 decoded in software (FFmpeg) - may not be real time.";
        case CODEC_VVC:  return "VVC/H.266 decoded in software (FFmpeg) - may not be real time.";
#else
        case CODEC_AV1:  return "AV1 has no PS4 hardware decoder - not supported.";
        case CODEC_VP9:  return "VP9 has no PS4 hardware decoder - not supported.";
        case CODEC_VVC:  return "VVC/H.266 has no PS4 hardware decoder - not supported.";
#endif
        default:         return "Codec could not be determined.";
    }
}

static int contains_ci(const char *hay, const char *needle)
{
    if (!hay || !needle) return 0;
    size_t nl = strlen(needle);
    for (const char *p = hay; *p; p++) {
        if (strncasecmp(p, needle, nl) == 0)
            return 1;
    }
    return 0;
}

StreamCodec codec_from_string(const char *s)
{
    if (!s || !*s) return CODEC_UNKNOWN;

    if (contains_ci(s, "hevc") || contains_ci(s, "h265") || contains_ci(s, "h.265") ||
        contains_ci(s, "hev1") || contains_ci(s, "hvc1"))
        return CODEC_H265;

    if (contains_ci(s, "avc")  || contains_ci(s, "h264") || contains_ci(s, "h.264") ||
        contains_ci(s, "x264"))
        return CODEC_H264;

    if (contains_ci(s, "av01") || contains_ci(s, "av1"))
        return CODEC_AV1;

    if (contains_ci(s, "vp09") || contains_ci(s, "vp9"))
        return CODEC_VP9;

    if (contains_ci(s, "vvc")  || contains_ci(s, "h266") || contains_ci(s, "h.266") ||
        contains_ci(s, "vvc1"))
        return CODEC_VVC;

    return CODEC_UNKNOWN;
}

StreamCodec codec_from_url_hint(const char *url)
{
    if (!url) return CODEC_UNKNOWN;
    // Container extensions do not identify a codec precisely, but common IPTV
    // conventions let us make a reasonable default guess for the UI.
    if (contains_ci(url, ".webm")) return CODEC_VP9;   // usually VP9/AV1
    if (contains_ci(url, ".mkv"))  return CODEC_H265;  // frequently HEVC
    if (contains_ci(url, ".m3u8")) return CODEC_H264;  // HLS defaults to AVC
    if (contains_ci(url, ".ts"))   return CODEC_H264;
    if (contains_ci(url, ".mp4"))  return CODEC_H264;
    return CODEC_UNKNOWN;
}
