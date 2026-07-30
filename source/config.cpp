#include "config.h"

#include <cstdio>
#include <sys/stat.h>
#include <string>
#include <sstream>

#define CONFIG_DIR  "/data/PS4-IPTV-Player"
#define CONFIG_FILE CONFIG_DIR "/sources.tsv"

int config_ensure_dir(void)
{
    struct stat st;
    if (stat(CONFIG_DIR, &st) == 0)
        return 0;
    if (mkdir(CONFIG_DIR, 0777) == 0)
        return 0;
    return -1;
}

static std::string escape(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\t') out += "\\t";
        else if (c == '\n') out += "\\n";
        else if (c == '\\') out += "\\\\";
        else out.push_back(c);
    }
    return out;
}

static std::string unescape(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[++i];
            if (n == 't') out.push_back('\t');
            else if (n == 'n') out.push_back('\n');
            else if (n == '\\') out.push_back('\\');
            else out.push_back(n);
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

std::vector<SourceProfile> config_load_sources(void)
{
    std::vector<SourceProfile> result;

    FILE *f = fopen(CONFIG_FILE, "rb");
    if (!f) return result;

    std::string content;
    char buf[4096];
    size_t rd;
    while ((rd = fread(buf, 1, sizeof(buf), f)) > 0)
        content.append(buf, rd);
    fclose(f);

    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;

        std::vector<std::string> cols;
        std::string cur;
        for (char c : line) {
            if (c == '\t') { cols.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        cols.push_back(cur);
        if (cols.size() < 7) continue;

        SourceProfile p;
        int kind = atoi(cols[0].c_str());
        p.kind     = (SourceKind)kind;
        p.name     = unescape(cols[1]);
        p.host     = unescape(cols[2]);
        p.username = unescape(cols[3]);
        p.password = unescape(cols[4]);
        p.url      = unescape(cols[5]);
        // cols[6] reserved for future fields
        result.push_back(std::move(p));
    }
    return result;
}

bool config_save_sources(const std::vector<SourceProfile> &sources)
{
    if (config_ensure_dir() != 0)
        return false;

    FILE *f = fopen(CONFIG_FILE, "wb");
    if (!f) return false;

    for (const auto &p : sources) {
        std::string line;
        line += std::to_string((int)p.kind);              line += '\t';
        line += escape(p.name);                           line += '\t';
        line += escape(p.host);                           line += '\t';
        line += escape(p.username);                       line += '\t';
        line += escape(p.password);                       line += '\t';
        line += escape(p.url);                            line += '\t';
        line += "";                                       line += '\n';
        fwrite(line.data(), 1, line.size(), f);
    }
    fclose(f);
    return true;
}
