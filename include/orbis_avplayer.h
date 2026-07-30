// ============================================================================
//  orbis_avplayer.h
//
//  Real prototypes / structures for the PS4 libSceAvPlayer module.
//
//  The OpenOrbis toolchain ships <orbis/AvPlayer.h> with NID stubs only
//  (every function is declared `void f()` with no arguments and none of the
//  data structures are present), which is not usable for real playback.
//  This header provides the actual public ABI so we can drive hardware video
//  decoding (H.264 / H.265) and demuxing of HTTP / HLS / local media sources.
//
//  Do NOT include <orbis/AvPlayer.h> in the same translation unit as this
//  header - the conflicting `void` declarations will not compile.
// ============================================================================
#ifndef PS4_IPTV_ORBIS_AVPLAYER_H
#define PS4_IPTV_ORBIS_AVPLAYER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SceAvPlayerImpl *SceAvPlayerHandle;

typedef enum SceAvPlayerTrickSpeeds {
    SCE_AVPLAYER_TRICK_SPEED_REWIND_32X   = -3200,
    SCE_AVPLAYER_TRICK_SPEED_REWIND_16X   = -1600,
    SCE_AVPLAYER_TRICK_SPEED_REWIND_8X    = -800,
    SCE_AVPLAYER_TRICK_SPEED_NORMAL       = 100,
    SCE_AVPLAYER_TRICK_SPEED_FF_8X        = 800,
    SCE_AVPLAYER_TRICK_SPEED_FF_16X       = 1600,
    SCE_AVPLAYER_TRICK_SPEED_FF_32X       = 3200
} SceAvPlayerTrickSpeeds;

typedef enum SceAvPlayerStreamType {
    SCE_AVPLAYER_VIDEO,
    SCE_AVPLAYER_AUDIO,
    SCE_AVPLAYER_TIMEDTEXT,
    SCE_AVPLAYER_UNKNOWN
} SceAvPlayerStreamType;

typedef enum SceAvPlayerSourceType {
    SCE_AVPLAYER_SOURCE_TYPE_UNKNOWN = 0,
    SCE_AVPLAYER_SOURCE_TYPE_FILE    = 1,
    SCE_AVPLAYER_SOURCE_TYPE_HLS     = 8
} SceAvPlayerSourceType;

typedef enum SceAvPlayerAVSyncMode {
    SCE_AVPLAYER_AV_SYNC_MODE_DEFAULT = 0,
    SCE_AVPLAYER_AV_SYNC_MODE_NONE
} SceAvPlayerAVSyncMode;

typedef enum SceAvPlayerState {
    SCE_AVPLAYER_STATE_STOP     = 0x00,
    SCE_AVPLAYER_STATE_READY    = 0x01,
    SCE_AVPLAYER_STATE_PLAY     = 0x02,
    SCE_AVPLAYER_STATE_PAUSE    = 0x03,
    SCE_AVPLAYER_STATE_BUFFERING= 0x04
} SceAvPlayerState;

// ---- Memory / IO callbacks -------------------------------------------------

typedef void *(*SceAvPlayerAllocate)(void *arg, uint32_t alignment, uint32_t size);
typedef void  (*SceAvPlayerDeallocate)(void *arg, void *ptr);
typedef void *(*SceAvPlayerAllocateTexture)(void *arg, uint32_t alignment, uint32_t size);
typedef void  (*SceAvPlayerDeallocateTexture)(void *arg, void *ptr);

typedef int  (*SceAvPlayerOpenFile)(void *arg, const char *filename);
typedef int  (*SceAvPlayerCloseFile)(void *arg);
typedef int  (*SceAvPlayerReadOffsetFile)(void *arg, uint8_t *buffer, uint64_t position, uint32_t length);
typedef uint64_t (*SceAvPlayerSizeFile)(void *arg);

typedef struct SceAvPlayerMemAllocator {
    void                        *objectPointer;
    SceAvPlayerAllocate          allocate;
    SceAvPlayerDeallocate        deallocate;
    SceAvPlayerAllocateTexture   allocateTexture;
    SceAvPlayerDeallocateTexture deallocateTexture;
} SceAvPlayerMemAllocator;

typedef struct SceAvPlayerFileReplacement {
    void                      *objectPointer;
    SceAvPlayerOpenFile        open;
    SceAvPlayerCloseFile       close;
    SceAvPlayerReadOffsetFile  readOffset;
    SceAvPlayerSizeFile        size;
} SceAvPlayerFileReplacement;

typedef void (*SceAvPlayerEventCallback)(void *p, int32_t argEventId, int32_t argSourceId, void *argEventData);

typedef struct SceAvPlayerEventReplacement {
    void                    *objectPointer;
    SceAvPlayerEventCallback eventCallback;
} SceAvPlayerEventReplacement;

typedef enum SceAvPlayerDebuglevels {
    SCE_AVPLAYER_DBG_NONE,
    SCE_AVPLAYER_DBG_INFO,
    SCE_AVPLAYER_DBG_WARNINGS,
    SCE_AVPLAYER_DBG_ALL
} SceAvPlayerDebuglevels;

typedef struct SceAvPlayerInitData {
    SceAvPlayerMemAllocator     memoryReplacement;
    SceAvPlayerFileReplacement  fileReplacement;
    SceAvPlayerEventReplacement eventReplacement;
    SceAvPlayerDebuglevels      debugLevel;
    uint32_t                    basePriority;
    int32_t                     numOutputVideoFrameBuffers;
    int32_t                     autoStart;
    uint8_t                     reserved[3];
    const char                 *defaultLanguage;
} SceAvPlayerInitData;

// ---- Stream / frame info ---------------------------------------------------

typedef struct SceAvPlayerAudio {
    uint16_t channelCount;
    uint8_t  reserved[2];
    uint32_t sampleRate;
    uint32_t size;
    uint8_t  languageCode[4];
} SceAvPlayerAudio;

typedef struct SceAvPlayerVideo {
    uint32_t width;
    uint32_t height;
    float    aspectRatio;
    uint8_t  languageCode[4];
} SceAvPlayerVideo;

typedef struct SceAvPlayerTextPosition {
    uint16_t top;
    uint16_t left;
    uint16_t bottom;
    uint16_t right;
} SceAvPlayerTextPosition;

typedef struct SceAvPlayerTimedText {
    uint8_t                 languageCode[4];
    uint16_t                textSize;
    uint16_t                fontSize;
    SceAvPlayerTextPosition position;
} SceAvPlayerTimedText;

typedef union SceAvPlayerStreamDetails {
    uint8_t              reserved[16];
    SceAvPlayerAudio     audio;
    SceAvPlayerVideo     video;
    SceAvPlayerTimedText subtitle;
} SceAvPlayerStreamDetails;

typedef struct SceAvPlayerFrameInfo {
    uint8_t                 *pData;
    uint8_t                  reserved[4];
    uint64_t                 timeStamp;
    SceAvPlayerStreamDetails details;
} SceAvPlayerFrameInfo;

typedef struct SceAvPlayerStreamInfo {
    uint32_t                 type;
    uint8_t                  reserved[4];
    SceAvPlayerStreamDetails details;
    uint64_t                 duration;
    uint64_t                 startTime;
} SceAvPlayerStreamInfo;

typedef struct SceAvPlayerAudioEx {
    uint16_t channelCount;
    uint8_t  reserved[2];
    uint32_t sampleRate;
    uint32_t size;
    uint8_t  languageCode[4];
    uint8_t  reserved1[64];
} SceAvPlayerAudioEx;

typedef struct SceAvPlayerVideoEx {
    uint32_t width;
    uint32_t height;
    float    aspectRatio;
    uint8_t  languageCode[4];
    uint32_t framerate;
    uint32_t cropLeftOffset;
    uint32_t cropRightOffset;
    uint32_t cropTopOffset;
    uint32_t cropBottomOffset;
    uint32_t pitch;
    uint8_t  lumaBitDepth;
    uint8_t  chromaBitDepth;
    uint8_t  videoFullRangeFlag;
    uint8_t  reserved1[37];
} SceAvPlayerVideoEx;

typedef union SceAvPlayerStreamDetailsEx {
    SceAvPlayerAudioEx   audio;
    SceAvPlayerVideoEx   video;
    SceAvPlayerTimedText subtitle;
    uint8_t              reserved1[80];
} SceAvPlayerStreamDetailsEx;

typedef struct SceAvPlayerFrameInfoEx {
    void                      *pData;
    uint8_t                    reserved[4];
    uint64_t                   timeStamp;
    SceAvPlayerStreamDetailsEx details;
} SceAvPlayerFrameInfoEx;

// Event ids delivered to SceAvPlayerEventCallback.
#define SCE_AVPLAYER_STATE_STOP_EVENT       0x01
#define SCE_AVPLAYER_STATE_READY_EVENT      0x02
#define SCE_AVPLAYER_STATE_PLAY_EVENT       0x03
#define SCE_AVPLAYER_STATE_PAUSE_EVENT      0x04
#define SCE_AVPLAYER_STATE_BUFFERING_EVENT  0x05
#define SCE_AVPLAYER_TIMED_TEXT_DELIVERY    0x10
#define SCE_AVPLAYER_WARNING_ID             0x20
#define SCE_AVPLAYER_ENCRYPTION             0x30
#define SCE_AVPLAYER_DRM_ERROR              0x40

// ---- API -------------------------------------------------------------------

SceAvPlayerHandle sceAvPlayerInit(SceAvPlayerInitData *data);
int32_t sceAvPlayerAddSource(SceAvPlayerHandle handle, const char *filename);
int32_t sceAvPlayerAddSourceEx(SceAvPlayerHandle handle, SceAvPlayerSourceType sourceType, const char *filename);
int32_t sceAvPlayerClose(SceAvPlayerHandle handle);
uint64_t sceAvPlayerCurrentTime(SceAvPlayerHandle handle);
int32_t sceAvPlayerDisableStream(SceAvPlayerHandle handle, uint32_t streamId);
int32_t sceAvPlayerEnableStream(SceAvPlayerHandle handle, uint32_t streamId);
int32_t sceAvPlayerGetAudioData(SceAvPlayerHandle handle, SceAvPlayerFrameInfo *frameInfo);
int32_t sceAvPlayerGetStreamInfo(SceAvPlayerHandle handle, uint32_t streamId, SceAvPlayerStreamInfo *info);
int32_t sceAvPlayerGetVideoData(SceAvPlayerHandle handle, SceAvPlayerFrameInfo *frameInfo);
int32_t sceAvPlayerGetVideoDataEx(SceAvPlayerHandle handle, SceAvPlayerFrameInfoEx *frameInfo);
int32_t sceAvPlayerJumpToTime(SceAvPlayerHandle handle, uint64_t offset);
int32_t sceAvPlayerPause(SceAvPlayerHandle handle);
int32_t sceAvPlayerResume(SceAvPlayerHandle handle);
int32_t sceAvPlayerSetLooping(SceAvPlayerHandle handle, uint32_t loopFlag);
int32_t sceAvPlayerSetTrickSpeed(SceAvPlayerHandle handle, int32_t trickSpeed);
int32_t sceAvPlayerStart(SceAvPlayerHandle handle);
int32_t sceAvPlayerStop(SceAvPlayerHandle handle);
int32_t sceAvPlayerStreamCount(SceAvPlayerHandle handle);
uint8_t sceAvPlayerIsActive(SceAvPlayerHandle handle);
int32_t sceAvPlayerPostInit(SceAvPlayerHandle handle, SceAvPlayerInitData *data);

#ifdef __cplusplus
}
#endif

#endif // PS4_IPTV_ORBIS_AVPLAYER_H
