#include "resource_manager.h"
#include "logger.h"
#include <cstdlib>
#include <filesystem>

namespace {
    void create_if_doesnt_exist(const std::string& path) {
        if(!std::filesystem::is_directory(path) || !std::filesystem::exists(path)) {
		    std::filesystem::create_directory(path);
	    }
    }
}

#if defined(_WIN64) || defined(_WIN32) // Windows platform specific

namespace {
    std::string SAVE_DIRECTORY;
    constexpr const char DELIM = '\\';
    std::string RESOURCE_DIRECTORY;
    std::string LOCAL_APPDATA_SETTINGS;
}

void ResourceManager::init() {
	SAVE_DIRECTORY = std::string(getenv("APPDATA")) + "\\TVTorrent";
	LOCAL_APPDATA_SETTINGS = std::string(getenv("LOCALAPPDATA")) + "\\gtk-3.0";
	if(std::filesystem::exists("res"))
	    RESOURCE_DIRECTORY = "res";
	else if(std::filesystem::exists("..\\res"))
	    RESOURCE_DIRECTORY = "..\\res";
	else if(std::filesystem::exists(R"(C:\Program Files (x86)\TVTorrent\res)"))
	    RESOURCE_DIRECTORY = R"(C:\Program Files (x86)\TVTorrent\res)";
    create_if_doesnt_exist(SAVE_DIRECTORY);
}

#else // Linux platform specific

namespace {
    std::string SAVE_DIRECTORY;
    constexpr static const char DELIM = '/';
    std::string RESOURCE_DIRECTORY = "/usr/local/share/TVTorrent/res";
}

void ResourceManager::init() {
	SAVE_DIRECTORY = std::string(getenv("HOME")) + "/.local/TVTorrent";
    if(std::filesystem::exists("res"))
        RESOURCE_DIRECTORY = "res";
    else if(std::filesystem::exists("../res"))
        RESOURCE_DIRECTORY = "../res";
    create_if_doesnt_exist(SAVE_DIRECTORY);
}

#endif

void ResourceManager::create_torrent_save(const std::unordered_map<TVItem*, TorrentHandler*>& map) {
    Logger::watcher w("Creating torrent save");
    Json::Value value;
	int i = 0;
	for(auto& tvw : map) {
		Json::Value info;
		auto item = tvw.first;
		info["name"] = (std::string)item->name;
		info["img_path"] = (std::string)item->img_path;
		info["default_path"] = (std::string)item->default_save_path;
		
		Json::Value torrents;
		int j = 0;
		for(auto& pair : tvw.second->m_Handles) {
		    torrents[j++] = pair.second.status().name;
		}

		info["torrents"] = torrents;

		value[i++] = info;
	}

	Json::StreamWriterBuilder builder;
	builder["indentation"] = "\t";
	std::string document = Json::writeString(builder, value);

    std::string path = get_save_path("item_data");

	std::ofstream of(path, std::ios::out | std::ios::trunc);
	of << document;
	of.close();
}

void ResourceManager::create_feed_save(const std::vector<Glib::ustring> &feeds, const std::vector<Feed::Filter*> &filters, const std::vector<std::string>& downloads) {
    Logger::watcher w("Create feed save");
    Json::Value value;
    Json::Value _feeds;
    Json::Value _filters;
    Json::Value _downloads;
    int i = 0;
    for(auto& feed : feeds) {
        _feeds[i++] = std::string(feed);
    }
    value["feeds"] = _feeds;
    i = 0;
    for(auto filter : filters) {
        Json::Value _filter;
        _filter["id"] = std::string(filter->id);
        _filter["name"] = std::string(filter->name);
        _filter["ver_pattern"] = std::string(filter->ver_pattern);
        _filter["tvw"] = std::string(filter->tvw);
        int j = 0;
        for(auto& feed : filter->feeds) {
            _filter["feeds"][j++] = feed;
        }
        _filters[i++] = _filter;
    }
    value["filters"] = _filters;

    i = 0;
    for(auto& download : downloads) {
        _downloads[i++] = download;
    }
    value["downloads"] = _downloads;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "\t";
    std::string document = Json::writeString(builder, value);
    std::string path = get_save_path("feed_data");

    std::ofstream of(path, std::ios::out | std::ios::trunc);
    of << document;
    of.close();
}

bool ResourceManager::get_torrent_save(Json::Value &root) {
    Logger::watcher w("Getting torrent save");
    Json::CharReaderBuilder builder;
	builder["collectComments"] = false;
	std::string errs;
    std::string path = get_save_path("item_data");
	std::ifstream savefile(path, std::ios::in);
    if(savefile.fail()) return false;
	bool ok = Json::parseFromStream(builder, savefile, &root, &errs);
    if(!ok) 
	    std::cout << errs << std::endl;
	savefile.close();
    return ok;
}

bool ResourceManager::get_feed_save(Json::Value &root) {

    Logger::watcher w("Getting feed save");

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::string errs;
    std::string path = get_save_path("feed_data");
    std::ifstream savefile(path, std::ios::in);
    if(savefile.fail()) return false;
    bool ok = Json::parseFromStream(builder, savefile, &root, &errs);
    if(!ok)
        std::cout << errs << std::endl;
    savefile.close();
    return ok;
}

Glib::ustring ResourceManager::get_resource_path(const Glib::ustring &name) {
    //create_if_doesnt_exist(RESOURCE_DIRECTORY);
    return Glib::ustring::format(RESOURCE_DIRECTORY, DELIM, name);
}

void ResourceManager::delete_file(const std::string &name) {
    Logger::watcher w("Deleting save file");
    try {
        auto path = SAVE_DIRECTORY + DELIM + "torrents" + DELIM + name;
        if(std::filesystem::remove(path)) {
	        Logger::info("Save deleted! " + path);
	    } else Logger::error("Save not deleted!");
    } catch(const std::filesystem::filesystem_error& err) {
			Logger::error(err.what());
	}
}

std::string ResourceManager::get_torrent_save_dir(bool delim) {
    std::string path = get_save_path("torrents");
    if(delim) path += DELIM;
    create_if_doesnt_exist(path);
    return path;
}

Glib::ustring ResourceManager::get_save_path(const Glib::ustring &name) {
    return Glib::ustring::format(SAVE_DIRECTORY, DELIM, name);
}

std::string ResourceManager::create_path(const std::string& fld, const std::string& flnm) {
    return fld + DELIM + flnm;
}

std::string ResourceManager::get_gtk_settings_path() {
    return LOCAL_APPDATA_SETTINGS + DELIM + "settings.ini";
}

