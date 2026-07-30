#include "model.h"
#include <unordered_map>

void Playlist::rebuildCategories()
{
    categories.clear();
    std::unordered_map<std::string, int> byName;

    for (int i = 0; i < (int)channels.size(); i++) {
        std::string g = channels[i].group.empty() ? std::string("Uncategorized")
                                                   : channels[i].group;
        auto it = byName.find(g);
        int catIdx;
        if (it == byName.end()) {
            catIdx = (int)categories.size();
            byName[g] = catIdx;
            Category c;
            c.id = g;
            c.name = g;
            categories.push_back(std::move(c));
        } else {
            catIdx = it->second;
        }
        categories[catIdx].channelIndices.push_back(i);
    }
}
