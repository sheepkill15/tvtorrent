//
// Created by simon on 2021. jan. 6..
//

#include <fstream>
#include "settings_manager.h"
#include "resource_manager.h"

void SettingsManager::init() {

    m_Settings.ul_limit = -1;
    m_Settings.dl_limit = -1;

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
    }
    settingsfile.close();

}

void SettingsManager::save() {
    Json::Value value;
    value["dl_limit"] = m_Settings.dl_limit;
    value["ul_limit"] = m_Settings.ul_limit;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "\t";
    std::string document = Json::writeString(builder, value);

    std::string path = ResourceManager::get_save_path("settings");
    std::ofstream of(path, std::ios::out | std::ios::trunc);
    of << document;
    of.close();
}
