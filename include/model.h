// ============================================================================
//  model.h - Core data model shared across the app (C++).
// ============================================================================
#ifndef PS4_IPTV_MODEL_H
#define PS4_IPTV_MODEL_H

#include <string>
#include <vector>
#include "codec.h"

// A single playable entry (live channel, movie or episode).
struct Channel {
    std::string id;          // provider stream id (Xtream) or synthesized id
    std::string name;        // display name
    std::string url;         // fully-resolved playback URL
    std::string logoUrl;     // channel logo / poster (may be empty)
    std::string group;       // category / group-title
    StreamCodec codec = CODEC_UNKNOWN;
};

// A named grouping of channels (Xtream category or M3U group-title).
struct Category {
    std::string          id;
    std::string          name;
    std::vector<int>     channelIndices; // indices into Playlist::channels
};

// The kind of source a playlist was loaded from.
enum class SourceKind {
    Xtream,
    M3uUrl,
    LocalFile
};

// A fully materialised playlist plus its category index.
struct Playlist {
    SourceKind            kind = SourceKind::LocalFile;
    std::string           title;
    std::vector<Channel>  channels;
    std::vector<Category> categories;

    void clear() {
        channels.clear();
        categories.clear();
        title.clear();
    }

    // Rebuild the category list from each channel's group field.
    void rebuildCategories();
};

// A saved source the user can pick from the home screen.
struct SourceProfile {
    SourceKind  kind = SourceKind::M3uUrl;
    std::string name;

    // Xtream
    std::string host;      // e.g. http://panel.example.com:8080
    std::string username;
    std::string password;

    // M3U URL / local file
    std::string url;       // remote URL or absolute local path
};

#endif // PS4_IPTV_MODEL_H
