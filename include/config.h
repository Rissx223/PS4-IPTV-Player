// ============================================================================
//  config.h - Persist the user's saved source profiles under /data.
//
//  Profiles are stored as tab-separated records in
//  /data/PS4-IPTV-Player/sources.tsv so they survive between launches.
// ============================================================================
#ifndef PS4_IPTV_CONFIG_H
#define PS4_IPTV_CONFIG_H

#include "model.h"
#include <vector>
#include <set>
#include <string>

// Ensure the data directory exists. Returns 0 on success.
int  config_ensure_dir(void);

// Load all saved source profiles (empty vector if none / on first run).
std::vector<SourceProfile> config_load_sources(void);

// Persist the full list of source profiles, replacing any previous file.
bool config_save_sources(const std::vector<SourceProfile> &sources);

// Favorites are stored as a flat set of channel playback URLs.
std::set<std::string> config_load_favorites(void);
bool config_save_favorites(const std::set<std::string> &favorites);

#endif // PS4_IPTV_CONFIG_H
