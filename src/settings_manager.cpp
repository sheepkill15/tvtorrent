//
// Created by simon on 2021. jan. 6..
//

#include <fstream>
#include "settings_manager.h"
#include "resource_manager.h"
#include "logger.h"

void SettingsManager::init() {

    m_Settings.ul_limit = -1;
    m_Settings.dl_limit = -1;
    m_Settings.close_to_tray = false;
    m_Settings.should_ask_exit = true;
    m_Settings.selected_theme = FlatRemix;
    m_Settings.on_torrent_finish = Nothing;

    Json::Value root;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::string errs;
    std::string path = ResourceManager::get_save_path("settings");
    std::ifstream settingsfile(path, std::ios::in);
    if(settingsfile.fail()) return;

    bool ok = Json::parseFromStream(builder, settingsfile, &root, &errs);
    if(!ok)
        ;
    else {
        m_Settings.dl_limit = root["dl_limit"].asFloat();
        m_Settings.ul_limit = root["ul_limit"].asFloat();
        m_Settings.close_to_tray = root["close_to_tray"].asBool();
        if(root.isMember("should_ask_exit"))
            m_Settings.should_ask_exit = root["should_ask_exit"].asBool();
        if(root.isMember("selected_theme")) {
            m_Settings.selected_theme = (Theme)root["selected_theme"].asInt();
        }
        if(root.isMember("on_torrent_finish")) {
            m_Settings.on_torrent_finish = (TorrentFinishAction)root["on_torrent_finish"].asInt();
        }
    }
    settingsfile.close();
}

void SettingsManager::save() {
    Logger::watcher w("Saving settings");
    Json::Value value;
    value["dl_limit"] = m_Settings.dl_limit;
    value["ul_limit"] = m_Settings.ul_limit;
    value["close_to_tray"] = m_Settings.close_to_tray;
    value["should_ask_exit"] = m_Settings.should_ask_exit;
    value["selected_theme"] = m_Settings.selected_theme;
    value["on_torrent_finish"] = m_Settings.on_torrent_finish;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "\t";
    std::string document = Json::writeString(builder, value);

    std::string path = ResourceManager::get_save_path("settings");
    std::ofstream of(path, std::ios::out | std::ios::trunc);
    of << document;
    of.close();

    path = ResourceManager::get_gtk_settings_path();
    Logger::info(path);
    std::ifstream gtk_s_file(path);
    if(gtk_s_file.fail()) return;
    std::string newfile;
    std::string temp;
    while(getline(gtk_s_file, temp)) {
        if(temp.find("gtk-theme-name") != std::string::npos) {
            temp = std::string("gtk-theme-name=") + theme_to_string[m_Settings.selected_theme];
        }
        temp += '\n';
        newfile += temp;
    }
    gtk_s_file.close();
    std::ofstream gtk_s_file_s(path);
    if(gtk_s_file_s.fail()) return;
    gtk_s_file_s << newfile;

}
