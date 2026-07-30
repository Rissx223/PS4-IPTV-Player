// ============================================================================
//  ffmpeg_player.cpp - Optional software-decode backend for VideoPlayer.
//
//  Compiled only when USE_FFMPEG is defined (see the Makefile's `ffmpeg`
//  target and tools/build_ffmpeg.sh). It decodes streams that the PS4 hardware
//  decoder cannot handle (AV1 / VP9 / VVC) using libavcodec + libavformat, and
//  colour-converts frames to BGRA (== SDL's ARGB8888 on little-endian) via
//  libswscale.
//
//  This path is CPU-bound and NOT expected to sustain real-time 1080p on PS4;
//  it exists so those codecs are decodable at all, and degrades gracefully.
// ============================================================================
#ifdef USE_FFMPEG

#include "player.h"

#include <cstring>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace {

struct FfCtx {
    AVFormatContext *fmt      = nullptr;
    AVCodecContext  *dec      = nullptr;
    SwsContext      *sws      = nullptr;
    AVPacket        *pkt      = nullptr;
    AVFrame         *frame    = nullptr;
    int              videoIdx = -1;
    int              swsW     = 0;
    int              swsH     = 0;
};

} // namespace

bool VideoPlayer::ffOpen(const std::string &url, StreamCodec /*codec*/)
{
    FfCtx *c = new FfCtx();

    // Networking / protocol whitelisting is decided by the FFmpeg build.
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "rw_timeout", "15000000", 0); // 15s
    av_dict_set(&opts, "user_agent", "PS4-IPTV-Player", 0);

    if (avformat_open_input(&c->fmt, url.c_str(), nullptr, &opts) < 0) {
        av_dict_free(&opts);
        delete c;
        m_state = State::Error;
        m_error = "FFmpeg: failed to open stream";
        return false;
    }
    av_dict_free(&opts);

    if (avformat_find_stream_info(c->fmt, nullptr) < 0) {
        avformat_close_input(&c->fmt);
        delete c;
        m_state = State::Error;
        m_error = "FFmpeg: no stream info";
        return false;
    }

    // Pick the first video stream, then look up its decoder. Avoids the
    // av_find_best_stream() decoder-out-param whose constness changed across
    // FFmpeg major versions.
    c->videoIdx = -1;
    for (unsigned i = 0; i < c->fmt->nb_streams; i++) {
        if (c->fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            c->videoIdx = (int)i;
            break;
        }
    }
    const AVCodec *decoder = nullptr;
    if (c->videoIdx >= 0)
        decoder = avcodec_find_decoder(
            c->fmt->streams[c->videoIdx]->codecpar->codec_id);
    if (c->videoIdx < 0 || !decoder) {
        avformat_close_input(&c->fmt);
        delete c;
        m_state = State::Error;
        m_error = "FFmpeg: no video stream / decoder";
        return false;
    }

    c->dec = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(c->dec, c->fmt->streams[c->videoIdx]->codecpar);
    c->dec->thread_count = 0; // auto

    if (avcodec_open2(c->dec, decoder, nullptr) < 0) {
        avcodec_free_context(&c->dec);
        avformat_close_input(&c->fmt);
        delete c;
        m_state = State::Error;
        m_error = "FFmpeg: failed to open decoder";
        return false;
    }

    c->pkt   = av_packet_alloc();
    c->frame = av_frame_alloc();

    m_ff    = c;
    m_state = State::Opening;
    m_error.clear();
    return true;
}

bool VideoPlayer::ffUpdate()
{
    FfCtx *c = (FfCtx *)m_ff;
    if (!c) return false;

    // Pull packets until a full video frame is decoded (bounded per call).
    for (int guard = 0; guard < 64; guard++) {
        int ret = avcodec_receive_frame(c->dec, c->frame);
        if (ret == 0) {
            int w = c->frame->width;
            int h = c->frame->height;
            if (w <= 0 || h <= 0) return false;

            if (!c->sws || c->swsW != w || c->swsH != h) {
                if (c->sws) sws_freeContext(c->sws);
                c->sws = sws_getContext(w, h, (AVPixelFormat)c->frame->format,
                                        w, h, AV_PIX_FMT_BGRA,
                                        SWS_BILINEAR, nullptr, nullptr, nullptr);
                c->swsW = w;
                c->swsH = h;
            }
            if (!c->sws) return false;

            if (w != m_width || h != m_height) {
                m_width  = w;
                m_height = h;
                m_argb.assign((size_t)w * h, 0xFF000000u);
            }

            uint8_t *dstData[4] = { (uint8_t *)m_argb.data(), nullptr, nullptr, nullptr };
            int      dstLine[4] = { w * 4, 0, 0, 0 };
            sws_scale(c->sws, c->frame->data, c->frame->linesize, 0, h,
                      dstData, dstLine);

            av_frame_unref(c->frame);
            m_state    = State::Playing;
            m_hasFrame = true;
            return true;
        }
        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
            return false;

        // Need more input: read the next packet from the video stream.
        if (av_read_frame(c->fmt, c->pkt) < 0) {
            if (m_state == State::Playing) m_state = State::Idle;
            return false;
        }
        if (c->pkt->stream_index == c->videoIdx)
            avcodec_send_packet(c->dec, c->pkt);
        av_packet_unref(c->pkt);
    }
    return false;
}

void VideoPlayer::ffStop()
{
    FfCtx *c = (FfCtx *)m_ff;
    if (!c) return;
    if (c->sws)   sws_freeContext(c->sws);
    if (c->frame) av_frame_free(&c->frame);
    if (c->pkt)   av_packet_free(&c->pkt);
    if (c->dec)   avcodec_free_context(&c->dec);
    if (c->fmt)   avformat_close_input(&c->fmt);
    delete c;
    m_ff = nullptr;
}

#endif // USE_FFMPEG
