//
// Created by simon on 2021. jan. 10..
//

#include <libtorrent/torrent_info.hpp>
#include "container.h"
#include "resource_manager.h"
#include "settings_manager.h"

void DataContainer::add_feed(const std::string &url) {
    m_Feeds.push_back(new Feed(url));
}

void DataContainer::add_filter() {
    m_Filters.push_back(new Feed::Filter { .internal_id = m_FilterId++ });
}

void DataContainer::add_downloaded(const std::string &name) {
    m_Downloaded.push_back(name);
}

void DataContainer::add_group(const Glib::ustring &name, const Glib::ustring &img_path, const Glib::ustring &default_path) {
    if(name.empty() || default_path.empty()) return;
    auto item = new TVItem { .name =  name, .img_path = img_path, .default_save_path = default_path };
    auto hndl = new TorrentHandler();
    m_Groups.insert(std::make_pair(item, hndl));

    hndl->subscribe_for_completed([](const lt::torrent_handle& stat) { DataContainer::get_manager().on_torrent_completed(stat); });
}

void DataContainer::remove_feed(size_t hash) {
    int i = 0;
    for(auto feed : m_Feeds) {
        if(*feed == hash) {
            m_Feeds.erase(m_Feeds.begin() + i);
            delete feed;
            return;
        }
        i++;
    }
}

void DataContainer::remove_filter(size_t id) {
    int i = 0;
    for(auto filter : m_Filters) {
        if(*filter == id) {
            m_Filters.erase(m_Filters.begin() + i);
            delete filter;
            return;
        }
        i++;
    }
}

void DataContainer::remove_group(size_t hash) {
    for(auto& pair : m_Groups) {
        if(pair.first->hash == hash) {
            delete pair.first;
            delete pair.second;
            m_Groups.erase(pair.first);
            break;
        }
    }
}

void DataContainer::init() {
    Json::Value root;

    bool ok = ResourceManager::get_torrent_save(root);

    if(ok) {
        for(auto & i : root) {
            if(!i.isMember("default_path")) continue;
            add_group(i["name"].asString(), i["img_path"].asString(), i["default_path"].asString());
            if(i.isMember("torrents")) {
                for(int j = 0; j < i["torrents"].size(); j++) {
                    std::string uri = ResourceManager::get_torrent_save_dir(true) + i["torrents"][j].asString();
                    size_t h = Unique::from_string(i["name"].asString());
                    add_torrent(h, uri);
                }
            }
        }
    }
    Json::Value feed_root;

    ok = ResourceManager::get_feed_save(feed_root);
    if(ok) {
        for(const auto & i : feed_root["feeds"]) {
            add_feed(i.asString());
        }
        for(const auto& i : feed_root["filters"]) {
            add_filter();
            auto filter =  m_Filters.back();
            filter->internal_id = m_FilterId++;
            filter->id = i["id"].asString();
            filter->name = i["name"].asString();
            filter->ver_pattern = i["ver_pattern"].asString();
            filter->tvw = i["tvw"].asString();
            if(i.isMember("feeds") && i["feeds"]) {
                for(const auto& j : i["feeds"]) {
                    if(j.is<size_t>())
                        filter->feeds.emplace_back(j.asLargestUInt());
                }
            }
        }

        for(const auto& i : feed_root["downloads"]) {
            m_Downloaded.push_back(i.asString());
        }
    }
    m_Manager = new DataManager();
}

void DataContainer::cleanup() {

    ResourceManager::create_torrent_save(m_Groups);

    std::vector<Glib::ustring> feeds;
    for(auto feed : m_Feeds) {
        feeds.emplace_back(feed->GetUrl());
    }
    ResourceManager::create_feed_save(feeds, m_Filters, m_Downloaded);

    SettingsManager::save();

    delete m_Manager;

    for(auto& pair : m_Groups) {
        delete pair.first;
        delete pair.second;
    }

    for(auto filter : m_Filters) {
        delete filter;
    }
    for(auto feed : m_Feeds) {
        delete feed;
    }
}

std::pair<TVItem*, TorrentHandler*> DataContainer::get_group(const Glib::ustring &name) {
    size_t hash = Unique::from_string(name);
    for(auto& pair : m_Groups) {
        if(pair.first->hash == hash) {
            return pair;
        }
    }
    return std::make_pair(nullptr, nullptr);
}

std::pair<TVItem *, TorrentHandler *> DataContainer::get_group(size_t hash) {
    for(auto& pair : m_Groups) {
        if(pair.first->hash == hash) {
            return pair;
        }
    }
    return std::make_pair(nullptr, nullptr);
}

void DataContainer::add_torrent(size_t hash, const std::string &url, const std::string &path) {
    auto group = get_group(hash);
    size_t torrId = Unique::from_string(url);
    for(const auto& torrent : group.first->torrents) {
        if(torrent.hash == torrId)
            return;
    }

    group.first->torrents.push_back({url, path});
    group.second->AddTorrent(url, path);
}

void DataContainer::remove_torrent(size_t hash, const lt::torrent_handle& hndl, const std::string& name, int index, bool remove_files) {
    auto group = get_group(hash);

    for(auto& pair : group.second->m_Handles) {
        if(pair.second == hndl) {
            group.second->RemoveTorrent(pair.first, remove_files);
            break;
        }
    }

    if(remove_files) {
        ResourceManager::delete_file(name);
    }

    group.first->torrents.erase(group.first->torrents.begin() + index);
}

Feed* DataContainer::get_feed(size_t first) {
    for(auto feed : m_Feeds) {
        if(*feed == first) {
            return feed;
        }
    }
    return nullptr;
}

void DataContainer::remove_torrent(const std::string & torr_name, bool remove_files) {
    size_t hash = Unique::from_string(torr_name);
    for(auto& group : get_groups()) {
        for(auto& pair : group.second->m_Handles) {
            if(pair.first == hash) {
                group.second->RemoveTorrent(hash, remove_files);
                return;
            }
        }
    }
}

void DataContainer::add_torrent(size_t hash, const std::string &path) {
    auto group = get_group(hash);
    lt::torrent_info info(path);
    if(!info.is_valid()) return;

    group.first->torrents.push_back({path, ""});
    group.second->AddTorrent(path, "");
}
