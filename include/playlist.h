// ============================================================================
//  playlist.h - High-level playlist loading (dispatch by source kind).
// ============================================================================
#ifndef PS4_IPTV_PLAYLIST_H
#define PS4_IPTV_PLAYLIST_H

#include "model.h"
#include <string>

struct LoadResult {
    bool        ok = false;
    std::string message;
};

// Read a whole local file into `text`. Returns false if it cannot be opened.
bool file_read_all(const std::string &path, std::string &text);

// Load a source profile into `out`, dispatching to the Xtream, remote-M3U or
// local-file loader as appropriate.
LoadResult playlist_load(const SourceProfile &profile, Playlist &out);

#endif // PS4_IPTV_PLAYLIST_H
