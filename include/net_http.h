// ============================================================================
//  net_http.h - Thin HTTP(S) client built on libSceNet / libSceSsl / libSceHttp.
//
//  Used to authenticate against Xtream panels and to fetch remote M3U
//  playlists. TLS certificate verification is intentionally relaxed because a
//  great many IPTV panels ship self-signed or misconfigured certificates; this
//  mirrors the behaviour of common IPTV clients.
// ============================================================================
#ifndef PS4_IPTV_NET_HTTP_H
#define PS4_IPTV_NET_HTTP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HttpResponse {
    int      status;   // HTTP status code, or negative on transport failure
    char    *body;     // malloc'd, NUL-terminated; caller frees with http_free
    size_t   length;   // number of bytes in body (excluding the NUL)
} HttpResponse;

// Load the required system modules and initialise net/ssl/http. Safe to call
// more than once. Returns 0 on success, negative on failure.
int  http_global_init(void);
void http_global_term(void);

// Perform a GET and buffer the whole response in memory. On success returns 0
// and fills *out (out->status may still be a non-200 HTTP code). The caller
// owns out->body and must release it with http_free().
int  http_get(const char *url, HttpResponse *out);

// GET straight to a file on disk (e.g. a large playlist). Returns 0 on success.
int  http_get_to_file(const char *url, const char *dst_path);

void http_free(HttpResponse *resp);

#ifdef __cplusplus
}
#endif

#endif // PS4_IPTV_NET_HTTP_H
