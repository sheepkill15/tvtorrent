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

    enum TorrentFinishAction {
        Nothing = 0,
        Stop = 1,
        DeleteTorrent = 2
    };

    struct Settings {
        float dl_limit;
        float ul_limit;
        bool should_ask_exit;
        bool close_to_tray;

        Theme selected_theme;
        TorrentFinishAction on_torrent_finish;
    };

    SettingsManager() = delete;
    ~SettingsManager() = default;

    static void init();
    static void save();
    inline static Settings& get_settings() { return  m_Settings; }

    static const int n_theme = 2;
    static const int n_tf_action = 3;

    static constexpr char theme_to_string[2][25] = {"FlatRemix", "Windows10Dark"};
    static constexpr char tf_action_to_string[3][25] = {"Nothing", "Stop", "DeleteTorrent"};

private:
    inline static Settings m_Settings;
};


#endif //TVTORRENT_SETTINGS_MANAGER_H
