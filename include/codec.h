// ============================================================================
//  codec.h - Pluggable stream-codec abstraction.
//
//  The PS4 has a fixed-function video decoder exposed through libSceVideodec2
//  and libSceAvPlayer. In practice that hardware path decodes AVC (H.264) and
//  HEVC (H.265) only. AV1, VP9 and VVC (H.266) have NO hardware decode block on
//  PS4 and cannot be decoded in real time in software on this hardware, so they
//  are represented here but reported as unsupported. Keeping every codec in one
//  table lets the UI show accurate capability info instead of failing silently.
// ============================================================================
#ifndef PS4_IPTV_CODEC_H
#define PS4_IPTV_CODEC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum StreamCodec {
    CODEC_UNKNOWN = 0,
    CODEC_H264,   // AVC   - hardware decode
    CODEC_H265,   // HEVC  - hardware decode
    CODEC_AV1,    // AV1   - no PS4 hardware decode
    CODEC_VP9,    // VP9   - no PS4 hardware decode
    CODEC_VVC,    // H.266 - no PS4 hardware decode
    CODEC_COUNT
} StreamCodec;

typedef enum CodecDecodePath {
    CODEC_PATH_NONE = 0,
    CODEC_PATH_HARDWARE,   // fixed-function decoder via AvPlayer / Videodec2
    CODEC_PATH_SOFTWARE    // reserved: not viable for real-time on PS4
} CodecDecodePath;

// Human readable short name, e.g. "H.265".
const char     *codec_name(StreamCodec c);
// True when the codec can actually be played back on PS4 hardware.
int             codec_is_supported(StreamCodec c);
// Which decode path (if any) services this codec.
CodecDecodePath codec_decode_path(StreamCodec c);
// A short explanation shown in the UI when a codec is unsupported.
const char     *codec_support_note(StreamCodec c);

// Best-effort codec detection from a codec string that may appear in an
// EXTINF / Xtream field (e.g. "hevc", "avc1.640028", "av01", "vp09", "vvc1").
StreamCodec     codec_from_string(const char *s);
// Best-effort detection from a stream URL / file extension as a fallback.
StreamCodec     codec_from_url_hint(const char *url);

#ifdef __cplusplus
}
#endif

#endif // PS4_IPTV_CODEC_H
