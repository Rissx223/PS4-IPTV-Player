#include "net_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <orbis/Http.h>
#include <orbis/Ssl.h>
#include <orbis/Net.h>
#include <orbis/Sysmodule.h>

#define HTTP_USER_AGENT "PS4-IPTV-Player/1.0 (PLAYSTATION 4)"
#define NET_POOLSIZE    (16 * 1024)
#define HTTP_CHUNK      (64 * 1024)

static int g_initialised = 0;
static int g_netMemId    = 0;
static int g_sslCtxId    = 0;
static int g_httpCtxId   = 0;

// Accept self-signed / mismatched certificates common to IPTV panels.
static int32_t accept_all_ssl(int32_t sslId, uint32_t verifyErr,
                              void *const sslCert[], int32_t certNum, void *userArg)
{
    (void)sslId; (void)verifyErr; (void)sslCert; (void)certNum; (void)userArg;
    return 1; // proceed
}

int http_global_init(void)
{
    if (g_initialised)
        return 0;

    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET) < 0)  return -1;
    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP) < 0) return -2;
    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL) < 0)  return -3;

    if (sceNetInit() < 0)
        return -4;

    int ret = sceNetPoolCreate("iptv-net", NET_POOLSIZE, 0);
    if (ret < 0) return -5;
    g_netMemId = ret;

    ret = sceSslInit(SSL_POOLSIZE);
    if (ret < 0) return -6;
    g_sslCtxId = ret;

    ret = sceHttpInit(g_netMemId, g_sslCtxId, LIBHTTP_POOLSIZE);
    if (ret < 0) return -7;
    g_httpCtxId = ret;

    g_initialised = 1;
    return 0;
}

void http_global_term(void)
{
    if (!g_initialised)
        return;
    sceHttpTerm(g_httpCtxId);
    sceSslTerm(g_sslCtxId);
    sceNetPoolDestroy(g_netMemId);
    g_initialised = 0;
}

// Open a request for a URL. Returns 0 and fills the handle triple on success.
static int open_request(const char *url, int *tpl_out, int *conn_out, int *req_out)
{
    int tpl = sceHttpCreateTemplate(g_httpCtxId, HTTP_USER_AGENT, ORBIS_HTTP_VERSION_1_1, 1);
    if (tpl < 0) return -1;

    sceHttpsSetSslCallback(tpl, accept_all_ssl, NULL);
    sceHttpSetConnectTimeOut(tpl, 15 * 1000 * 1000); // 15s

    int conn = sceHttpCreateConnectionWithURL(tpl, url, 1);
    if (conn < 0) { sceHttpDeleteTemplate(tpl); return -2; }

    int req = sceHttpCreateRequestWithURL(conn, ORBIS_METHOD_GET, url, 0);
    if (req < 0) { sceHttpDeleteConnection(conn); sceHttpDeleteTemplate(tpl); return -3; }

    *tpl_out = tpl; *conn_out = conn; *req_out = req;
    return 0;
}

static void close_request(int tpl, int conn, int req)
{
    if (req  > 0) sceHttpDeleteRequest(req);
    if (conn > 0) sceHttpDeleteConnection(conn);
    if (tpl  > 0) sceHttpDeleteTemplate(tpl);
}

int http_get(const char *url, HttpResponse *out)
{
    if (!url || !out) return -1;
    out->status = -1;
    out->body   = NULL;
    out->length = 0;

    if (http_global_init() != 0)
        return -1;

    int tpl = 0, conn = 0, req = 0;
    if (open_request(url, &tpl, &conn, &req) != 0)
        return -1;

    int rc = -1;
    int32_t statusCode = 0;

    if (sceHttpSendRequest(req, NULL, 0) < 0)                goto done;
    if (sceHttpGetStatusCode(req, &statusCode) < 0)          goto done;
    out->status = statusCode;

    {
        size_t cap = HTTP_CHUNK;
        size_t len = 0;
        char  *buf = (char *)malloc(cap + 1);
        if (!buf) goto done;

        for (;;) {
            if (len + HTTP_CHUNK + 1 > cap) {
                cap *= 2;
                char *nb = (char *)realloc(buf, cap + 1);
                if (!nb) { free(buf); buf = NULL; break; }
                buf = nb;
            }
            int read = sceHttpReadData(req, buf + len, HTTP_CHUNK);
            if (read < 0)  { free(buf); buf = NULL; break; }
            if (read == 0) break; // EOF
            len += (size_t)read;
        }

        if (buf) {
            buf[len]    = '\0';
            out->body   = buf;
            out->length = len;
            rc = 0;
        }
    }

done:
    close_request(tpl, conn, req);
    return rc;
}

int http_get_to_file(const char *url, const char *dst_path)
{
    if (!url || !dst_path) return -1;
    if (http_global_init() != 0) return -1;

    int tpl = 0, conn = 0, req = 0;
    if (open_request(url, &tpl, &conn, &req) != 0)
        return -1;

    int rc = -1;
    int32_t statusCode = 0;
    FILE *fd = NULL;

    if (sceHttpSendRequest(req, NULL, 0) < 0)       goto done;
    if (sceHttpGetStatusCode(req, &statusCode) < 0) goto done;
    if (statusCode != 200)                          goto done;

    fd = fopen(dst_path, "wb");
    if (!fd) goto done;

    {
        static uint8_t chunk[HTTP_CHUNK];
        for (;;) {
            int read = sceHttpReadData(req, chunk, sizeof(chunk));
            if (read < 0)  goto done;
            if (read == 0) { rc = 0; break; }
            if (fwrite(chunk, 1, (size_t)read, fd) != (size_t)read) goto done;
        }
    }

done:
    if (fd) fclose(fd);
    close_request(tpl, conn, req);
    return rc;
}

void http_free(HttpResponse *resp)
{
    if (resp && resp->body) {
        free(resp->body);
        resp->body   = NULL;
        resp->length = 0;
    }
}
