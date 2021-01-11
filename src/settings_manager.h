//
// Created by simon on 2021. jan. 6..
//

#ifndef TVTORRENT_SETTINGS_MANAGER_H
#define TVTORRENT_SETTINGS_MANAGER_H

class SettingsManager {
public:
    enum Theme {
        FlatRemix = 0,
        Windows10Dark = 1
    };
    struct Settings {
        float dl_limit;
        float ul_limit;
        bool should_ask_exit;
        bool close_to_tray;

        Theme selected_theme;
    };

    SettingsManager() = delete;
    ~SettingsManager() = default;

    static void init();
    static void save();
    inline static Settings& get_settings() { return  m_Settings; }

    static const int n_theme = 2;

    static constexpr char theme_to_string[2][25] = {"FlatRemix", "Windows10Dark"};

private:
    inline static Settings m_Settings;
};


#endif //TVTORRENT_SETTINGS_MANAGER_H
