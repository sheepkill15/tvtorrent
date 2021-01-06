//
// Created by simon on 2021. jan. 6..
//

#ifndef TVTORRENT_SETTINGS_MANAGER_H
#define TVTORRENT_SETTINGS_MANAGER_H


class SettingsManager {
public:
    struct Settings {
        float dl_limit;
        float ul_limit;
    };

    SettingsManager() = delete;
    ~SettingsManager() = default;

    static void init();
    static void save();
    inline static Settings& get_settings() { return  m_Settings; }

private:
    inline static Settings m_Settings;
};


#endif //TVTORRENT_SETTINGS_MANAGER_H
