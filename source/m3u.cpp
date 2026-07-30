#include "m3u.h"
#include <cctype>

namespace {

std::string trim(const std::string &s)
{
    size_t a = 0, b = s.size();
    while (a < b && (unsigned char)s[a] <= ' ') a++;
    while (b > a && (unsigned char)s[b - 1] <= ' ') b--;
    return s.substr(a, b - a);
}

// Extract the value of attr="..." from an EXTINF attribute string.
std::string attr(const std::string &line, const std::string &key)
{
    std::string needle = key + "=\"";
    size_t pos = line.find(needle);
    if (pos == std::string::npos) return std::string();
    pos += needle.size();
    size_t endq = line.find('"', pos);
    if (endq == std::string::npos) return std::string();
    return line.substr(pos, endq - pos);
}

// The display name is whatever follows the last comma of the EXTINF line.
std::string displayName(const std::string &extinf)
{
    size_t comma = extinf.rfind(',');
    if (comma == std::string::npos) return std::string();
    return trim(extinf.substr(comma + 1));
}

} // namespace

int m3u_parse(const std::string &text, Playlist &out)
{
    out.channels.clear();

    Channel pending;
    bool havePending = false;
    std::string codecHint;

    size_t i = 0;
    const size_t n = text.size();

    while (i < n) {
        size_t nl = text.find('\n', i);
        std::string line = trim(text.substr(i, (nl == std::string::npos ? n : nl) - i));
        i = (nl == std::string::npos) ? n : nl + 1;

        if (line.empty()) continue;
        if (line[0] == '#') {
            if (line.rfind("#EXTINF", 0) == 0) {
                pending = Channel();
                pending.name    = displayName(line);
                pending.id      = attr(line, "tvg-id");
                pending.logoUrl = attr(line, "tvg-logo");
                pending.group   = attr(line, "group-title");
                std::string tvgName = attr(line, "tvg-name");
                if (pending.name.empty()) pending.name = tvgName;
                // Some providers annotate the codec in a custom attribute.
                codecHint = attr(line, "codec");
                if (codecHint.empty()) codecHint = attr(line, "tvg-codec");
                havePending = true;
            } else if (line.rfind("#EXTGRP:", 0) == 0) {
                if (havePending) pending.group = trim(line.substr(8));
            }
            // #EXTM3U and unknown tags ignored.
            continue;
        }

        // A non-comment line is the media URL for the pending EXTINF entry.
        if (!havePending) {
            pending = Channel();
            pending.name = line;
        }
        pending.url = line;

        pending.codec = codec_from_string(codecHint.c_str());
        if (pending.codec == CODEC_UNKNOWN)
            pending.codec = codec_from_url_hint(pending.url.c_str());

        if (pending.name.empty()) pending.name = pending.url;
        out.channels.push_back(pending);

        havePending = false;
        codecHint.clear();
    }

    out.rebuildCategories();
    return (int)out.channels.size();
}
