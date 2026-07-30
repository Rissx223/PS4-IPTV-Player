// ============================================================================
//  xtream.h - Xtream Codes API client (IPTV panel login).
//
//  Implements the subset of the player_api.php protocol needed to browse and
//  play a panel: authentication, live TV categories/streams and VOD
//  categories/streams. Stream playback URLs are built with the documented
//  path scheme (/live/<user>/<pass>/<id>.<ext>, /movie/...).
// ============================================================================
#ifndef PS4_IPTV_XTREAM_H
#define PS4_IPTV_XTREAM_H

#include "model.h"
#include <string>

struct XtreamResult {
    bool        ok = false;
    std::string message;   // error / status text for the UI
};

// Authenticate against the panel. On success out.ok is true and userInfo /
// serverInfo are filled with the raw status text for display.
XtreamResult xtream_authenticate(const SourceProfile &profile, std::string &status);

// Authenticate and load the full playlist (live + VOD) into `out`.
XtreamResult xtream_load(const SourceProfile &profile, Playlist &out);

// Best-effort "now / next" EPG for a live stream. Titles are base64-decoded.
// Returns false on any error; `now` / `next` are left untouched then.
bool xtream_short_epg(const SourceProfile &profile, const std::string &streamId,
                      std::string &now, std::string &next);

#endif // PS4_IPTV_XTREAM_H
