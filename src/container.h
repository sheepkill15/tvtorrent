//
// Created by simon on 2021. jan. 10..
//

#ifndef TVTORRENT_CONTAINER_H
#define TVTORRENT_CONTAINER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "feed.h"
#include "tv_item.h"
#include "torrent_handler.h"
#include "data_manager.h"

class DataContainer {
public:
    DataContainer() = delete;
    ~DataContainer() = default;

    static void init();
    static void cleanup();

    static void add_feed(const std::string& url);
    static void add_filter();
    static void add_downloaded(const std::string& name);
    static void add_group(const Glib::ustring& name, const Glib::ustring& img_path, const Glib::ustring& default_path);
    static void add_torrent(size_t hash, const std::string& url, const std::string& path);
    static void add_torrent(size_t hash, const std::string& url);

    static std::pair<TVItem*, TorrentHandler*> get_group(const Glib::ustring& name);
    static std::pair<TVItem*, TorrentHandler*> get_group(size_t hash);

    static void remove_feed(size_t hash);
    static void remove_filter(size_t id);
    static void remove_group(size_t hash);
    static void remove_torrent(size_t hash, const lt::torrent_handle&, const std::string&, int index, bool remove_files = false);
    static void remove_torrent(const std::string&, bool remove_files = false);

    inline static const std::vector<Feed*>& get_feeds()  { return m_Feeds; };
    inline static const std::vector<Feed::Filter*>& get_filters()  { return m_Filters; };
    inline static const std::vector<std::string>& get_downloaded()  { return m_Downloaded; };
    inline static const std::unordered_map<TVItem*, TorrentHandler*>& get_groups() { return m_Groups; }
    inline static std::set<size_t>& get_notified() { return m_AlreadyNotified; };

    static Feed* get_feed(size_t first);

    inline static DataManager& get_manager() { return *m_Manager; };

private:
    inline static std::vector<Feed*> m_Feeds{};
    inline static std::vector<std::string> m_Downloaded{};
    inline static std::vector<Feed::Filter*> m_Filters{};
    inline static std::unordered_map<TVItem*, TorrentHandler*> m_Groups{};

    inline static size_t m_FilterId = 0;

    inline static DataManager* m_Manager{};

    inline static std::set<size_t> m_AlreadyNotified;
};


#endif //TVTORRENT_CONTAINER_H
