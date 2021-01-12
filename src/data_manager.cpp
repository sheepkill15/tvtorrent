//
// Created by simon on 2021. jan. 12..
//

#include "data_manager.h"
#include "container.h"
#include "settings_manager.h"

namespace {
    void ReplaceAll(std::string &str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
    }
    size_t split(const std::string &txt, std::vector<std::string> &strs, char ch)
    {
        size_t pos = txt.find( ch );
        size_t initialPos = 0;
        strs.clear();

        // Decompose statement
        while( pos != std::string::npos ) {
            strs.push_back( txt.substr( initialPos, pos - initialPos ) );
            initialPos = pos + 1;

            pos = txt.find( ch, initialPos );
        }

        // Add the last one
        strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

        return strs.size();
    }
}

DataManager::DataManager() {
    paused = false;
    check = std::thread([this] { check_feeds(); });
    for(auto& group : DataContainer::get_groups()) {
        group.second->subscribe_for_completed([this](const lt::torrent_handle& stat) { DataManager::on_torrent_completed(stat); });
    }

}

DataManager::~DataManager() {

    should_work = false;
    check.detach();

}

void DataManager::refresh_check() {

    should_work = false;
    check.detach();
    check = std::thread([this] { check_feeds(); });
}

void DataManager::check_feeds() {
    should_work = true;
    while(should_work) {
        {
            if(paused) goto skip;
            for(auto& filter : DataContainer::get_filters()) {
                if(filter->tvw.empty() || filter->name.empty()) continue;
                std::string filter_processed = "\\b" + filter->ver_pattern;
                ReplaceAll(filter_processed, "X", "[0-9]");
                filter_processed += "\\b";
                std::regex re(filter_processed);
                std::vector<std::string> name_split;
                split(filter->name, name_split, ' ');
                for(auto& feed : DataContainer::get_feeds()) {
                    for(auto& item : feed->GetItems()) {
                        bool ok = true;
                        for(auto& s : name_split) {
                            if(item.title.find(s) == std::string::npos) {
                                ok = false;
                                break;
                            }
                        }
                        if(ok) {
                            std::smatch m;
                            bool const matched = std::regex_search(item.title, m, re);
                            if(matched) {
                                bool const ifAlreadyDownloaded = check_if_already_downloaded(name_split, m);
                                if(!ifAlreadyDownloaded) {
                                    DataContainer::add_downloaded(item.title);
                                    for(auto& group : DataContainer::get_groups()) {
                                        if(group.first->name == filter->tvw) {
                                            DataContainer::add_torrent(group.first->hash, item.link, group.first->default_save_path);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }
        skip:

        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}

bool DataManager::check_if_already_downloaded(const std::vector<std::string> &name, const std::smatch &m) {
    for(auto& item : DataContainer::get_downloaded()) {
        bool ok = true;
        for(auto& s : name) {
            if(item.find(s) == std::string::npos) {
                ok = false;
            }
        }
        if(ok && (m[0].str().empty() || item.find(m[0]) != std::string::npos)) {
            return true;
        }
    }
    return false;
}

void DataManager::set_notify_callback(std::function<void(const std::string &)> cb) {
    m_Notify_Callback = std::move(cb);
}

void DataManager::pause() {
    paused = true;
}

void DataManager::resume() {
    paused = false;
}

void DataManager::on_torrent_completed(const lt::torrent_handle &handle) {
    if(m_Notify_Callback)
        m_Notify_Callback(handle.status().name);
    switch(SettingsManager::get_settings().on_torrent_finish) {
        case SettingsManager::Stop:
            handle.unset_flags(lt::torrent_flags::auto_managed);
            handle.pause();
            break;
        case SettingsManager::DeleteTorrent:
            DataContainer::remove_torrent(handle.status().name, false);
            break;
        case SettingsManager::Nothing:
        default:
            break;
    }
}
