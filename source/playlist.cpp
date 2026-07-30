#include "playlist.h"
#include "m3u.h"
#include "xtream.h"
#include "net_http.h"

#include <cstdio>

bool file_read_all(const std::string &path, std::string &text)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return false; }

    text.resize((size_t)sz);
    size_t rd = fread(&text[0], 1, (size_t)sz, f);
    fclose(f);
    text.resize(rd);
    return true;
}

static LoadResult loadM3uText(const std::string &text, SourceKind kind,
                              const std::string &title, Playlist &out)
{
    LoadResult r;
    out.clear();
    out.kind  = kind;
    out.title = title;

    int n = m3u_parse(text, out);
    if (n <= 0) {
        r.message = "No channels found in playlist";
        return r;
    }
    r.ok = true;
    r.message = "Loaded " + std::to_string(n) + " channels";
    return r;
}

LoadResult playlist_load(const SourceProfile &profile, Playlist &out)
{
    LoadResult r;

    switch (profile.kind) {
        case SourceKind::Xtream: {
            XtreamResult xr = xtream_load(profile, out);
            r.ok = xr.ok;
            r.message = xr.message;
            return r;
        }
        case SourceKind::M3uUrl: {
            HttpResponse resp;
            if (http_get(profile.url.c_str(), &resp) != 0) {
                r.message = "Failed to download playlist";
                return r;
            }
            if (resp.status != 200) {
                r.message = "HTTP status " + std::to_string(resp.status);
                http_free(&resp);
                return r;
            }
            std::string text = resp.body ? resp.body : "";
            http_free(&resp);
            return loadM3uText(text, SourceKind::M3uUrl,
                               profile.name.empty() ? "M3U Playlist" : profile.name, out);
        }
        case SourceKind::LocalFile: {
            std::string text;
            if (!file_read_all(profile.url, text)) {
                r.message = "Cannot open file: " + profile.url;
                return r;
            }
            return loadM3uText(text, SourceKind::LocalFile,
                               profile.name.empty() ? "Local Playlist" : profile.name, out);
        }
    }

    r.message = "Unknown source kind";
    return r;
}
