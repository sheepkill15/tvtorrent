#ifndef TVTORRENT_RESOURCE_MANAGER_H
#define TVTORRENT_RESOURCE_MANAGER_H

#include "glibmm/ustring.h"
#include "tv_widget.h"
#include <vector>
#include <json/json.h>

class ResourceManager {
public:
    ResourceManager() = delete;
    static void init();

    static void create_torrent_save(const std::vector<TVWidget*>& tvw_list );
    static Glib::ustring get_resource_path(const Glib::ustring& name);
    static Glib::ustring get_save_path(const Glib::ustring& name);
    static bool get_torrent_save(Json::Value& root);
    static void delete_file(const std::string& path);

    static void create_feed_save(const std::vector<Glib::ustring>& feeds, const std::vector<Feed::Filter>& filters, const std::vector<std::string>& downloads);
    static bool get_feed_save(Json::Value& root);

    static std::string get_torrent_save_dir(bool delim = false);

    static std::string create_path(const std::string& fld, std::string flnm);

};

#endif