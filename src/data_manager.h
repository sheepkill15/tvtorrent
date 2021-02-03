//
// Created by simon on 2021. jan. 12..
//

#ifndef TVTORRENT_DATA_MANAGER_H
#define TVTORRENT_DATA_MANAGER_H

#include <string>
#include <vector>
#include <regex>
#include <thread>
#include <functional>
#include "torrent_handler.h"

class DataManager {
public:
    DataManager();
    ~DataManager();

    void set_notify_callback(std::function<void(const std::string&)>);
    void refresh_check();
    void pause();
    void resume();
    void on_torrent_completed(const lt::torrent_handle& handle);


private:

    std::function<void(const std::string&)> m_Notify_Callback;

    std::thread check;
    bool paused{};
    bool should_work{};

    void check_feeds();
    static bool check_if_already_downloaded(const std::vector<std::string>&, const std::smatch&);

};


#endif //TVTORRENT_DATA_MANAGER_H
