// ============================================================================
//  m3u.h - Extended M3U / M3U8 playlist parser.
//
//  Understands the #EXTINF attribute conventions used by IPTV providers:
//  tvg-id, tvg-name, tvg-logo, group-title, and an optional codec hint. Lines
//  are tolerated in any order and unknown tags are ignored.
// ============================================================================
#ifndef PS4_IPTV_M3U_H
#define PS4_IPTV_M3U_H

#include "model.h"
#include <string>

// Parse extended M3U text into the playlist (channels + categories).
// Returns the number of channels parsed.
int m3u_parse(const std::string &text, Playlist &out);

#endif // PS4_IPTV_M3U_H
