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

    static void create_save(const std::vector<TVWidget*>& tvw_list );
    static Glib::ustring get_resource_path(const Glib::ustring& name);
    static bool get_save(Json::Value& root);
    static void delete_file(const std::string& path);
    static void create_file(const std::string& path);
    static void delete_file_with_path(const std::string& path, const std::string& name);
    static std::string get_torrent_save_dir(bool delim = false);

private:

};

#endif