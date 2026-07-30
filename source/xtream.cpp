#include "xtream.h"
#include "json.h"
#include "net_http.h"

#include <string>
#include <unordered_map>

namespace {

// Strip a trailing slash so we can append path segments uniformly.
std::string normalizeHost(const std::string &host)
{
    std::string h = host;
    if (!h.empty() && h.back() == '/')
        h.pop_back();
    return h;
}

std::string urlEncode(const std::string &s)
{
    static const char *hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size() * 3);
    for (unsigned char c : s) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back((char)c);
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0xF]);
        }
    }
    return out;
}

std::string apiUrl(const SourceProfile &p, const std::string &action)
{
    std::string url = normalizeHost(p.host);
    url += "/player_api.php?username=" + urlEncode(p.username) +
           "&password=" + urlEncode(p.password);
    if (!action.empty())
        url += "&action=" + action;
    return url;
}

bool getJson(const std::string &url, JsonValue &out, std::string &err)
{
    HttpResponse resp;
    if (http_get(url.c_str(), &resp) != 0) {
        err = "Network request failed";
        return false;
    }
    if (resp.status != 200) {
        err = "HTTP status " + std::to_string(resp.status);
        http_free(&resp);
        return false;
    }
    std::string body = resp.body ? resp.body : "";
    http_free(&resp);

    if (!json_parse(body, out)) {
        err = "Invalid JSON from panel";
        return false;
    }
    return true;
}

// Fetch categories keyed by category_id -> category_name.
void loadCategories(const SourceProfile &p, const std::string &action,
                    std::unordered_map<std::string, std::string> &names)
{
    JsonValue v;
    std::string err;
    if (!getJson(apiUrl(p, action), v, err)) return;
    if (!v.isArray()) return;
    for (size_t i = 0; i < v.size(); i++) {
        const JsonValue &c = v.at(i);
        std::string id   = c["category_id"].asString();
        std::string name = c["category_name"].asString();
        if (!id.empty())
            names[id] = name.empty() ? id : name;
    }
}

void appendStreams(const SourceProfile &p, const std::string &action,
                   const std::unordered_map<std::string, std::string> &catNames,
                   const char *pathSegment, bool isVod, Playlist &out)
{
    JsonValue v;
    std::string err;
    if (!getJson(apiUrl(p, action), v, err)) return;
    if (!v.isArray()) return;

    std::string host = normalizeHost(p.host);

    for (size_t i = 0; i < v.size(); i++) {
        const JsonValue &s = v.at(i);

        Channel ch;
        ch.id      = s["stream_id"].asString();
        ch.name    = s["name"].asString();
        ch.logoUrl = isVod ? s["stream_icon"].asString() : s["stream_icon"].asString();

        std::string catId = s["category_id"].asString();
        auto it = catNames.find(catId);
        ch.group = (it != catNames.end()) ? it->second : std::string("Uncategorized");

        // Container extension: live is TS, VOD carries its own extension.
        std::string ext = isVod ? s["container_extension"].asString() : std::string("ts");
        if (ext.empty()) ext = isVod ? "mp4" : "ts";

        ch.url = host + "/" + pathSegment + "/" +
                 urlEncode(p.username) + "/" + urlEncode(p.password) + "/" +
                 ch.id + "." + ext;

        // Codec is not exposed by the API; infer a sensible default.
        ch.codec = codec_from_url_hint(ch.url.c_str());

        if (ch.name.empty()) ch.name = "Stream " + ch.id;
        if (!ch.id.empty())
            out.channels.push_back(std::move(ch));
    }
}

} // namespace

XtreamResult xtream_authenticate(const SourceProfile &profile, std::string &status)
{
    XtreamResult r;
    JsonValue v;
    std::string err;

    if (!getJson(apiUrl(profile, ""), v, err)) {
        r.message = err;
        return r;
    }

    const JsonValue &userInfo = v["user_info"];
    std::string auth = userInfo["auth"].asString();
    std::string authStatus = userInfo["status"].asString();

    if (auth == "1" || authStatus == "Active") {
        r.ok = true;
        r.message = "Authenticated";
        status = "Status: " + (authStatus.empty() ? std::string("Active") : authStatus) +
                 "  Exp: " + userInfo["exp_date"].asString() +
                 "  Conns: " + userInfo["active_cons"].asString() + "/" +
                 userInfo["max_connections"].asString();
    } else {
        r.message = authStatus.empty() ? "Authentication failed" : authStatus;
    }
    return r;
}

XtreamResult xtream_load(const SourceProfile &profile, Playlist &out)
{
    out.clear();
    out.kind  = SourceKind::Xtream;
    out.title = "Xtream: " + profile.name;

    std::string status;
    XtreamResult auth = xtream_authenticate(profile, status);
    if (!auth.ok)
        return auth;

    std::unordered_map<std::string, std::string> liveCats, vodCats;
    loadCategories(profile, "get_live_categories", liveCats);
    loadCategories(profile, "get_vod_categories",  vodCats);

    appendStreams(profile, "get_live_streams", liveCats, "live",  false, out);
    appendStreams(profile, "get_vod_streams",  vodCats,  "movie", true,  out);

    out.rebuildCategories();

    XtreamResult r;
    r.ok = true;
    r.message = "Loaded " + std::to_string(out.channels.size()) + " streams";
    return r;
}
